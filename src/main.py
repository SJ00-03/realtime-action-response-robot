#************************************#
# 2026 비전 응용 프로젝트
# 팀 : 구성최광임
#************************************#

import sys
import os
import time
import cv2
import csv
import queue
import multiprocessing
import threading
import supervision as sv
from config import *
from utils.utils import draw_landmarks_on_full_frame
from utils.rapi_logics import is_attack, is_smoke, is_falldown

# workers
from workers.hailo_worker import hailo_worker
from workers.mediapipe_worker import mediapipe_worker
from workers.hardware_worker import hardware_worker

# 테스트용으로 멀티 스레딩 or 멀티 프로세싱 설정 가능
if USE_MULTIPROCESSING:
    WorkerProcess = multiprocessing.Process
    WorkerQueue = multiprocessing.Queue
else:
    WorkerProcess = threading.Thread
    WorkerQueue = queue.Queue


def main():
    ''' 메인 프로세스, 프로세스를 프로세서에 할당 및 화면에 렌더링 등 진행'''
    print(f"[INFO] {'MULTIPROCESSING' if USE_MULTIPROCESSING else 'MULTITHREADING'}")
    print("[INFO] start main processor...")
    
    # 데이터 통신을 위한 queue 정의
    frame_queue = WorkerQueue(maxsize=2)            # Main -> Hailo (대기열 짧게 유지하여 지연 방지)
    cropped_queue = WorkerQueue(maxsize=10)         # Hailo -> MediaPipe
    result_queue = WorkerQueue(maxsize=30)          # Hailo, MediaPipe -> Main (결과 취합)
    servo_command_queue = WorkerQueue(maxsize=30)   # Main -> Hardware
    web_queue = WorkerQueue(maxsize=20)             # Main -> Hardware
    web_video_queue = WorkerQueue(maxsize=50)       # Main -> Hardware

    # 프로세스 or 스레드 생성 및 실행
    hailo_proc = WorkerProcess(target=hailo_worker, args=(frame_queue, cropped_queue, result_queue, HEF_PATH), daemon=True)
    pose_proc = WorkerProcess(target=mediapipe_worker, args=(cropped_queue, result_queue, MODEL_PATH), daemon=True)
    hardware_proc = WorkerProcess(target=hardware_worker, args=(servo_command_queue, web_queue, web_video_queue), daemon=True) 
    hailo_proc.start()
    pose_proc.start()
    hardware_proc.start()
    
    # 카메라 설정
    cap = cv2.VideoCapture(WEBCAM_ID)
    cap.set(cv2.CAP_PROP_FRAME_WIDTH, 640)
    cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 480)
    
    if not cap.isOpened():
        print("cam not found")
        return
    
    # 시스템의 state를 저장
    state = {
        'latest_detections': sv.Detections.empty(),
        'track_history': {}, 
        'orig_w': 640, 
        'missing_frames': 0,
        'missing_object_frames': {}, 
        "last_valid_command": {}, 
        'missing_counter' : 0,
        'cached_poses': {} 
    }
    
    frame_count = 0
    start_time = time.time()
    performance_metrics = [] 
    frame_capture_times = {} 
    last_fps_calc_time = time.time()
    hailo_frame_count = 0
    pose_frame_count = 0
    hailo_fps_display = 0.0 
    pose_fps_display = 0.0
    main_fps_display = 0.0
    
    csv_filename = f"performance_metrics_{"MultiProcessing" if USE_MULTIPROCESSING else "MultiThreading"}_{time.strftime('%H_%M')}.csv"
    file_exists = os.path.isfile(csv_filename)
    
    FIRST = True
    
    print("[INFO] start main loop")
    while True:
        ret, frame = cap.read()
        if not ret: break
        
        frame_count += 1
        capture_time = time.time()
        frame_capture_times[frame_count] = capture_time 
        annotated_frame = frame.copy()

        # main -> hailo 프레임을 queue에 넣기
        if not frame_queue.full():
            try: 
                frame_queue.put_nowait((frame_count, frame, capture_time))
            except queue.Full: 
                pass
        
        current_latency = 0 # 레이턴시 측정용 변수 초기화
        
        # worker 부터 결과(result_queue) 얻기 (비동기)
        while not result_queue.empty():
            try:
                msg = result_queue.get_nowait()
                msg_type = msg.get('type')
                
                # 레이턴시 측정
                if 'capture_time' in msg:
                    current_latency = (time.time() - msg['capture_time']) * 1000
                # queue에서 받아온 결과로 state 업데이트
                if msg_type == 'hailo':
                    hailo_frame_count += 1
                    state['latest_detections'] = msg['detections']
                    state['track_history'] = msg['track_history'] 
                    state['orig_w'] = msg['orig_w']
                elif msg_type == 'pose':
                    pose_frame_count += 1
                    state['cached_poses'][msg['track_id']] = (msg['landmarks'], msg['bbox_margin'])
            except queue.Empty: break

        detections = state['latest_detections']
        history = state['track_history']
        orig_w = state['orig_w']
        
        # 웹 서버 및 하드웨어로 보낼 데이터 초기화
        web_message = {
            "situation_types": { "person" : 0, "falldown" : 0, "attack" : 0,"smoking" : 0}
            }
        command_message = {
            "trackings": {},"command": {"command_person":0, "command_person_warn":0,"command_attack":0, "command_attack_warn":0,"command_falldown":0, "command_falldown_warn":0,"command_smoking":0, "command_smoking_warn":0}
        }
        
        active_track_ids = []
        current_targets = {}
        
        # 객체를 detect하면 저장
        if len(detections) > 0:
            for i, (xyxy, cls_id) in enumerate(zip(detections.xyxy, detections.class_id)): 
                if detections.tracker_id is None or detections.tracker_id[i] is None: continue
                track_id = int(detections.tracker_id[i])
                current_targets[track_id] = {"bbox": xyxy.astype(int), "class_id": int(cls_id)}
                
                # msissing 초기화
                state['missing_object_frames'][track_id] = 0
                state['missing_frames'] = 0
        
        # 트래킹 중이던 객체가 일정 프레임 이하 동안 안보일 경우 이전 위치 유지
        for track_id, item_data in history.items():
            if track_id not in current_targets:
                missing_count = state['missing_object_frames'].get(track_id, 0) + 1
                state['missing_object_frames'][track_id] = missing_count
                state['missing_frames'] += 1
                if missing_count < 15 and "last_bbox" in item_data: 
                    current_targets[track_id] = {"bbox": item_data["last_bbox"], "class_id": item_data.get("class_id", 0)}
        
        
        # 객체 행동(person, falldown attack, smoke, ) 판단 로직
        for track_id, target_data in current_targets.items():
            active_track_ids.append(track_id)
            x1, y1, x2, y2 = target_data["bbox"]
            class_id_val = target_data["class_id"]
            cls_name = CLASS_NAMES.get(class_id_val, f"Unknown_{class_id_val}")
            
            # 화면에 박스 그리기
            cv2.rectangle(annotated_frame, (x1, y1), (x2, y2), (48, 48, 255), 2)
            cv2.putText(annotated_frame, f"ID:{track_id} {cls_name}", (x1, max(15, y1 - 10)), cv2.FONT_HERSHEY_SIMPLEX, .8, (0, 0, 255), 2)
            web_message["situation_types"]["person"] += 1 # 박스를 그린다는건 사람이 detect 된것이라 1 증가
            
            # 화면 표시용 거리 및 위치 판별
            tracking_box_height = abs(y1 - y2)
            box_center_relative = ((x1 + x2) / 2) / orig_w
            
            dist_txt = "normal"
            pos_txt = "center"
            
            if tracking_box_height > 430:
                dist_txt = "close" 
            elif tracking_box_height > 180 and tracking_box_height < 430:
                dist_txt = "normal"
            else:
                dist_txt = "far"
                
            if box_center_relative < LEFT_LIMIT:
                pos_txt = "left"
            elif box_center_relative > RIGHT_LIMIT:
                pos_txt = "RIGHT"
            else: 
                pos_txt = "CENTER"
            
            text_y = y1 + 25 if y1 > 30 else y1 + 45
            cv2.putText(annotated_frame, f"{dist_txt} | {pos_txt}", (x1 + 5, text_y), cv2.FONT_HERSHEY_SIMPLEX, 0.8, (0,0,255), 2)
            
            
            
            # 미디어파이프 데이터 연동 및 이벤트 판별
            item = history.get(track_id, {})
            attack_frames = item.get("attack_frames", 0)
            smoke_frames = item.get("smoke_frames", 0)
            falldown_frames = item.get("falldown_frames", 0)
            person_frames = item.get("person_frames", 0)
            
            
            # 현재 track_id의 뼈대 데이터가 현재상태(캐시)(state)에 있다면 판별 시작
            if track_id in state['cached_poses']:
                landmarks, bbox_margin = state['cached_poses'][track_id]
                if len(landmarks) > 0:
                    draw_landmarks_on_full_frame(annotated_frame, landmarks, bbox_margin)
                    try:
                        # 1. person 감지
                        if person_frames > 10:
                            command_message["command"]["command_person_warn"] += 1
                            cv2.putText(annotated_frame, "person", (10, 300), cv2.FONT_HERSHEY_SIMPLEX, 1.2, (0, 0, 255), 3)
                            if person_frames > 50: command_message["command"]["command_person"] += 1

                        # 2. falldown 감지
                        if falldown_frames > 10 or class_id_val == 1:
                            if is_falldown(landmarks, bbox_margin):
                                cv2.putText(annotated_frame, "FALLDOWN", (10, 300), cv2.FONT_HERSHEY_SIMPLEX, 1.2, (0, 0, 255), 3)
                                web_message["situation_types"]["falldown"] += 1
                                command_message["command"]["command_falldown_warn"] += 1
                                if falldown_frames > 25: command_message["command"]["command_falldown"] += 1     
                                                         
                        # 3. attack 감지
                        if attack_frames > 10 or class_id_val == 2:
                            if is_attack(landmarks) or class_id_val == 2:
                                cv2.putText(annotated_frame, "ATTACK", (10, 300), cv2.FONT_HERSHEY_SIMPLEX, 1.2, (0, 0, 255), 3)
                                web_message["situation_types"]["attack"] += 1
                                command_message["command"]["command_attack_warn"] += 1
                                if attack_frames > 25: command_message["command"]["command_attack"] += 1
                        # 4. smoking 감지
                        if smoke_frames > 10 or class_id_val == 3:
                            if is_smoke(landmarks):
                                cv2.putText(annotated_frame, "SMOKING!", (10, 300), cv2.FONT_HERSHEY_SIMPLEX, 1.2, (0, 0, 255), 3)
                                web_message["situation_types"]["smoking"] += 1
                                command_message["command"]["command_smoking_warn"] += 1
                                if smoke_frames > 65: command_message["command"]["command_smoking"] += 1

                    except Exception as e: print("[ERROR]", e)
                    
            # 서보로 움직임 명령              
            command_message["trackings"][track_id] = {
                "bbox":[int(x1), int(y1), int(x2), int(y2)], 
                "class_id" : class_id_val, 
                "class_name" : cls_name
            }
            
        # 화면에서 사라진 객체들의 state에서 삭제                    
        for trackid in list(state['cached_poses'].keys()):
            if trackid not in active_track_ids:
                del state['cached_poses'][trackid]
                if trackid in state['missing_object_frames']: 
                    del state['missing_object_frames'][trackid]
                    
        # 추적 중인 객체 있으면 명령 보내기. 없으면 일정 프레임 동안 이전에 보냈던 명령 보내기
        if active_track_ids:
            state['missing_counter'] = 0
            state['last_valid_command'] = command_message.copy()
        else:
            state['missing_counter'] = state.get('missing_counter', 0) + 1
            
            # 저장된게 있으면서 missing_counter가 30 이하면 저장된거 보내기
            if state['missing_counter'] < 30 and state.get('last_valid_command'):
                command_message = state['last_valid_command']
            # 초기화해서 명령 보내기
            else:
                command_message = {"trackings": {},"command": {"command_person":0, "command_person_warn":0,"command_falldown":0,"command_falldown_warn":0, "command_attack":0,"command_attack_warn":0,"command_smoking":0,"command_smoking_warn":0,}}
                state['last_valid_command'] = None 
        
        # Main -> Hardware: 명령 데이터 queue에 넣기
        while not servo_command_queue.empty():
            try: servo_command_queue.get_nowait()    
            except queue.Empty: break
        try: servo_command_queue.put(command_message)
        except queue.Full: pass
        
        # Main -> Hardware: 웹서버에 전솔할 데이터 queue에 넣기
        while not web_queue.empty():
            try: web_queue.get_nowait()    
            except queue.Empty: break
        try: web_queue.put(web_message)
        except queue.Full: pass
        

        # 프레임 계산
        current_time = time.time()
        time_diff = current_time - last_fps_calc_time
        if time_diff >= 1.0:
            main_fps_display = frame_count / (current_time - start_time)
            hailo_fps_display = hailo_frame_count / time_diff
            pose_fps_display = pose_frame_count / time_diff
            hailo_frame_count, pose_frame_count, last_fps_calc_time = 0, 0, current_time
            
        cv2.putText(annotated_frame, f"Main  : {int(main_fps_display)} FPS", (10, 30), cv2.FONT_HERSHEY_SIMPLEX, 0.8, (0, 0, 255), 2)
        cv2.putText(annotated_frame, f"YOLO : {int(hailo_fps_display)} FPS", (10, 60), cv2.FONT_HERSHEY_SIMPLEX, 0.8, (0, 0, 255), 2)
        cv2.putText(annotated_frame, f"Pose : {int(pose_fps_display)} FPS", (10, 90), cv2.FONT_HERSHEY_SIMPLEX, 0.8, (0, 0, 255), 2)
        
        # 성능 지표 기록
        if current_latency > 0:
            performance_metrics.append((frame_count, main_fps_display, hailo_fps_display, pose_fps_display, current_latency,  command_message["command"]["command_person"], command_message["command"]["command_falldown"], command_message["command"]["command_attack"], command_message["command"]["command_smoking"]))
        
        
        cv2.imshow("Main (Lightweight Orchestrator)", annotated_frame)
        if cv2.waitKey(1) & 0xFF == ord('q'): break

        # 웹 스트리밍용 queue로 최종 프레임 전달
        web_video_message = {
            "frame_count": frame_count, 
            "image": annotated_frame.copy()
            }
        while not web_video_queue.empty():
            try: web_video_queue.get_nowait()
            except queue.Empty: break
        try: web_video_queue.put_nowait(web_video_message)
        except queue.Full: pass

        # fps 관련 데이터 csv 저장
        if (frame_count % 1000 == 0) and DEBUG_MODE:
            with open(csv_filename, 'a', newline='') as f:
                writer = csv.writer(f)
                if not file_exists and FIRST:
                    writer.writerow(['Frame', 'Main_FPS','YOLO_FPS','Pose_FPS', 'Latency(ms)', 'person','falldown','attack','smoking'])
                    FIRST = False
                writer.writerows(performance_metrics)
                performance_metrics = []
            print(f"[INFO] Saved to {csv_filename}")
            
        if frame_count > MAX_TEST_FRAMES: break

    print("\n[INFO] shutdown.")
    cap.release()
    cv2.destroyAllWindows()
    
    # 화면 종료 후 남은 queue 비우기
    while not frame_queue.empty(): frame_queue.get()
    while not cropped_queue.empty(): cropped_queue.get()
    while not servo_command_queue.empty(): servo_command_queue.get()
    while not web_queue.empty(): web_queue.get()
    while not web_video_queue.empty() : web_video_queue.get()
    
    frame_queue.put(None); 
    cropped_queue.put(None); 
    servo_command_queue.put(None)
    web_queue.put(None); 
    web_video_queue.put(None)
    
    hailo_proc.join(timeout=2.0)
    pose_proc.join(timeout=2.0)
    hardware_proc.join(timeout=2.0)
    print("[INFO] end.")

if __name__ == "__main__":
    multiprocessing.set_start_method('spawn', force=True)
    main()