import time
import cv2
import numpy as np
import queue
import supervision as sv

from hailo_platform import (
    HEF, VDevice, HailoStreamInterface, InferVStreams, 
    ConfigureParams, InputVStreamParams, OutputVStreamParams, FormatType
)
from utils.core_utils import letterbox_resize, get_person_crops

def hailo_worker(frame_queue, cropped_queue, result_queue, hef_path, CONF_THRESH=0.1):
    """카메라로부터 프레임을 받아 Hailo에서 객체 추론을 담당하는 프로세스"""
    print("[Hailo Worker] initializing...")
    time.sleep(1)
    
    # 1. hef 파일 불러오기
    hef = HEF(hef_path) 
    
    # 2. 트래킹으로 ByteTrack 사용
    tracker = sv.ByteTrack(
        track_activation_threshold=0.12, # threshold 이상이어야 시작
        lost_track_buffer=180,           # 화면에서 사라져도 180프레임 동안은 기억함
        minimum_matching_threshold=0.4,  # 이전 박스와 현재 박스가 40% 이상 겹쳐야 같은 객체
        frame_rate=30                    # 카메라 fps
    )
    
    track_history = {} # 추적 중인 객체 저장

    # 3. hailo 가속기 연결 설정
    with VDevice() as target:
        """ https://github.com/cj-mills/pytorch-yolox-object-detection-tutorial-code/blob/main/scripts/yolox-hailo-bytetrack-rpi.py """
        # Configure the device with the HEF and PCIe interface
        configure_params = ConfigureParams.create_from_hef(hef, interface=HailoStreamInterface.PCIe)
        network_groups = target.configure(hef, configure_params)
        
        # Select the first network group
        network_group = network_groups[0]
        network_group_params = network_group.create_params()
        
        # Create input and output virtual streams params
        input_vstreams_params = InputVStreamParams.make_from_network_group(network_group, quantized=False, format_type=FormatType.FLOAT32)
        output_vstreams_params = OutputVStreamParams.make_from_network_group(network_group, quantized=False, format_type=FormatType.FLOAT32)
        
        # Get information about the input and output virtual streams
        input_vstream_info = hef.get_input_vstream_infos()[0]
        output_vstream_info = hef.get_output_vstream_infos()[0]
        
        input_name = input_vstream_info.name
        input_shape = input_vstream_info.shape 
        
        
        # 가끔 헤일로 추론에서 원하는 형식으로 안 들어와서 이거 사용
        if len(input_shape) == 4:
            # (배치 크기, 세로, 가로, 색상)
            model_h, model_w = input_shape[1], input_shape[2]
        else:
            # (세로, 가로, 색상)
            model_h, model_w = input_shape[0], input_shape[1]
        
        print("[Hailo Worker] ready")
        
        # 4. 가속기 활성화 및 추론 시작
        with network_group.activate(network_group_params):
            with InferVStreams(network_group, input_vstreams_params, output_vstreams_params) as infer_pipeline:
                while True:
                    try:
                        # queue에서 프레임 가져옴
                        data = frame_queue.get(timeout=2.0)
                        if data is None: 
                            break
                        
                        frame_count, frame, capture_time = data
                        orig_h, orig_w = frame.shape[:2]
                        
                        # 헤일로 추론을 위해 프레임 색상 변경
                        rgb_frame = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB) # bgr 로 받아오는거 rgb로
                        
                        # 헤일로 가속기에 맞게 프레임이 찌그러지지 않게 비율을 유지하려고 래터박스 진행
                        resized_frame, ratio, pad_w, pad_h =  letterbox_resize(rgb_frame, (model_w, model_h))
                        
                        # 헤일로에 맞는 형식으로 변환
                        input_data = np.expand_dims(resized_frame, axis=0).astype(np.float32)
                        input_data = np.ascontiguousarray(input_data)
                        
                        # 추론 결과 받아옴
                        infer_results = infer_pipeline.infer({input_name: input_data})
                        
                        # 결과를 리스트로 변환
                        output_key = list(infer_results.keys())[0]
                        batch_detections = infer_results[output_key][0] # detection 결과들 받아오기
                    
                        xyxy_list, conf_list, class_id_list = [], [], []
                        
                        # 후처리: 아까 래터박스해서 추론했던 결과를 원본 으미지 크기로 바꾸기
                        for class_id_idx, class_boxes in enumerate(batch_detections):
                            for box in class_boxes:
                                if len(box) == 0: 
                                    continue
                                y_min, x_min, y_max, x_max, conf = box[:5]
                                
                                if conf < CONF_THRESH: 
                                    continue
                                
                                # (640,640)에 맞게 픽셀 좌표로 변환
                                box_xmin = x_min * model_w
                                box_ymin = y_min * model_h
                                box_xmax = x_max * model_w
                                box_ymax = y_max * model_h

                                # (640,640) 레터박스 패딩을 빼고, 비율로 다시 나눠서 원본 카메라 해상도 좌표(640,480)로 복구
                                x1 = int((box_xmin - pad_w) / ratio)
                                y1 = int((box_ymin - pad_h) / ratio)
                                x2 = int((box_xmax - pad_w) / ratio)
                                y2 = int((box_ymax - pad_h) / ratio)
                                
                                # 변환된 박스의 최대 최솟값 제한
                                x1 = max(0, x1)
                                y1 = max(0, y1)
                                x2 = min(orig_w, x2)
                                y2 = min(orig_h, y2)
                                
                                xyxy_list.append([x1, y1, x2, y2])
                                conf_list.append(float(conf))
                                class_id_list.append(int(class_id_idx))

                        # 객체추적: supervision 라이브러리로 tracking
                        if len(xyxy_list) > 0:
                            detections = sv.Detections(
                                xyxy=np.array(xyxy_list, dtype=np.float32),
                                confidence=np.array(conf_list, dtype=np.float32),
                                class_id=np.array(class_id_list, dtype=np.int32)
                            )
                            tracked_detections = tracker.update_with_detections(detections)
                            
                        else:
                            tracked_detections = sv.Detections.empty()

                        # 미디어파이프에서 크롭된 박스 내에서만 찾도록 하기 위해 사용(성능을 위해)
                        person_crops = get_person_crops(frame, tracked_detections, track_history, margin=40)
                        
                        
                        # hailo_worker -> midiapipe_worker
                        for item in person_crops:
                            if not cropped_queue.full():
                                cropped_queue.put_nowait((
                                    item["track_id"], 
                                    item["crop"], 
                                    item["bbox_margin"], 
                                    capture_time
                                    ))

                        
                        # hailo_worker -> main 화면에 그릴거 main으로 보내기
                        hailo_result = {
                            'type': 'hailo', 
                            'frame_count': frame_count, 
                            'capture_time': capture_time,
                            'detections': tracked_detections, 
                            'track_history': track_history.copy(), 
                            'orig_w': orig_w
                        }
                        result_queue.put(hailo_result, block=False)
                        
                    except queue.Empty: 
                        continue
                    except queue.Full: 
                        continue
                    except Exception as e: 
                        print("[ERROR] Hailo Worker", e)