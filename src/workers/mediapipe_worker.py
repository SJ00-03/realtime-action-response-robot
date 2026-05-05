import time
import queue
import cv2
import mediapipe as mp

def mediapipe_worker(cropped_queue, result_queue, model_path):
    """Crop된 이미지를 받아 MediaPipe 뼈대 연산하는 프로세스"""
    
    print("[MediaPipe Worker] initalizing...")
    time.sleep(2) # 다른 프로세스와 동시에 실행되면 전원 공급 초과떄문에 딜레이 설정
    BaseOptions = mp.tasks.BaseOptions
    PoseLandmarker = mp.tasks.vision.PoseLandmarker
    options = mp.tasks.vision.PoseLandmarkerOptions(
        base_options=BaseOptions(model_asset_path=model_path), 
        running_mode=mp.tasks.vision.RunningMode.VIDEO
    )
    
    with PoseLandmarker.create_from_options(options) as landmarker:
        timestamp_ms = 0
        print("[MediaPipe Worker] ready")
        while True:
            try:
                # hailo 프로세스로부터 온 크롭된거 가져오기
                data = cropped_queue.get(timeout=2.0)
                if data is None: 
                    break
                track_id, crop, bbox_margin, capture_time = data
                
                
                rgb_frame = cv2.cvtColor(crop, cv2.COLOR_BGR2RGB)
                mp_image = mp.Image(image_format=mp.ImageFormat.SRGB, data=rgb_frame)
                
                timestamp_ms+=1
                pose_result = landmarker.detect_for_video(mp_image, timestamp_ms)
                
                landmarks_data = []
                if pose_result.pose_landmarks:
                    for lm in pose_result.pose_landmarks[0]:
                        landmarks_data.append({'x': lm.x, 'y': lm.y})
                
                pose_result_dict = {
                    'type': 'pose', 
                    'track_id': track_id, 
                    'landmarks': landmarks_data, 
                    'bbox_margin': bbox_margin, 
                    'capture_time': capture_time
                }
                # mediapipe -> main 관절 정보 보내기
                result_queue.put(pose_result_dict, block=False)
                
            except queue.Empty: 
                continue
            except queue.Full: 
                continue
            except Exception as e:
                print("[ERROR]", e)