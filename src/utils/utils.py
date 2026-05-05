import cv2
from config import POSE_CONNECTIONS

def clamping(v, low, high):
    # low - v - high
    return max(low, min(v, high))

def letterbox_resize(img, new_shape=(640, 640), color=(114, 114, 114)):
    # 원본 이미지 비율 유지하며 리사이즈 및 패딩 추가
    shape = img.shape[:2]
    r = min(new_shape[0] / shape[0], new_shape[1] / shape[1])
    new_unpad = int(round(shape[1] * r)), int(round(shape[0] * r))
    
    dw, dh = new_shape[1] - new_unpad[0], new_shape[0] - new_unpad[1]
    dw, dh = dw / 2, dh / 2 

    if shape[::-1] != new_unpad:
        img = cv2.resize(img, new_unpad, interpolation=cv2.INTER_LINEAR)
    
    top, bottom = int(round(dh - 0.1)), int(round(dh + 0.1))
    left, right = int(round(dw - 0.1)), int(round(dw + 0.1))
    img = cv2.copyMakeBorder(img, top, bottom, left, right, cv2.BORDER_CONSTANT, value=color)
    
    return img, r, dw, dh

def draw_landmarks_on_full_frame(frame, pose_landmarks, bbox_margin):
    x1_margin, y1_margin, x2_margin, y2_margin = bbox_margin
    w = x2_margin - x1_margin
    h = y2_margin - y1_margin

    for connection in POSE_CONNECTIONS:
        start_index, end_index = connection
        if start_index >= len(pose_landmarks) or end_index >= len(pose_landmarks):
            continue
        start_x = x1_margin + int(pose_landmarks[start_index]['x'] * w)
        start_y = y1_margin + int(pose_landmarks[start_index]['y'] * h)
        end_x = x1_margin + int(pose_landmarks[end_index]['x'] * w)
        end_y = y1_margin + int(pose_landmarks[end_index]['y'] * h)
        cv2.line(frame, (start_x, start_y), (end_x, end_y), (255, 0, 0), 2)

    for lm in pose_landmarks:
        cx = x1_margin + int(lm['x'] * w)
        cy = y1_margin + int(lm['y'] * h)
        cv2.circle(frame, (cx, cy), 3, (0, 255, 0), -1)

def get_person_crops(frame, tracked_detections, track_history, target_class_id=0, conf_thresh=0.2, margin=20, max_miss_frames=3, required_frames=3):
    h, w = frame.shape[:2]
    crops = []
    current_detected_ids = set()
    
    falldown_reset = 0
    attack_reset = 0
    smoking_reset = 0
        
    for i in range(len(tracked_detections)):
        if tracked_detections.tracker_id is None or tracked_detections.tracker_id[i] is None: continue
        cls_id = int(tracked_detections.class_id[i])
        track_id = int(tracked_detections.tracker_id[i])
        current_detected_ids.add(track_id)
        x1, y1, x2, y2 = tracked_detections.xyxy[i].astype(int)

        if track_id not in track_history:
            track_history[track_id] = {
                "class_id": -1, 
                "bbox": (x1, y1, x2, y2), 
                "missed_frames": 0, 
                "hit_frames": 1, 
                "person_frames":0, "falldown_frames": 0, "attack_frames": 0, "smoke_frames": 0,
                "person_miss": 0, "falldown_miss": 0, "attack_miss": 0, "smoke_miss": 0,
            }
        else:
            track_history[track_id]["bbox"] = (x1, y1, x2, y2)
            track_history[track_id]["missed_frames"] = 0
            track_history[track_id]["hit_frames"] += 1 

        if track_history[track_id]["hit_frames"] >= required_frames:
            x1m = clamping(x1 - margin, 0, w - 1)
            y1m = clamping(y1 - margin, 0, h - 1)
            x2m = clamping(x2 + margin, 0, w - 1)
            y2m = clamping(y2 + margin, 0, h - 1)
            
            if cls_id == 0: 
                track_history[track_id]["person_frames"] += 1 
                track_history[track_id]["person_miss"] = 0
                if track_history[track_id]["person_frames"] > 10: 
                    track_history[track_id]["class_id"] = 0
                
            elif cls_id == 1: 
                track_history[track_id]["falldown_frames"] += 1; 
                track_history[track_id]["falldown_miss"] = 0
                if track_history[track_id]["falldown_frames"] > 10: 
                    track_history[track_id]["class_id"] = 1
                    falldown_reset = 0
                    
            elif cls_id == 2: 
                track_history[track_id]["attack_frames"] += 1
                track_history[track_id]["attack_miss"] = 0
                if track_history[track_id]["attack_frames"] > 10: 
                    track_history[track_id]["class_id"] = 2
                    attack_reset = 0
            
            elif cls_id == 3: 
                track_history[track_id]["smoke_frames"] += 1
                track_history[track_id]["smoke_miss"] = 0
                if track_history[track_id]["smoke_frames"] > 10: 
                    track_history[track_id]["class_id"] = 3
                    smoking_reset = 0
            
                
            # miss 되면 초기화
            if cls_id != 0:
                track_history[track_id]["person_miss"] += 1
                if track_history[track_id]["person_miss"] > 10:
                    track_history[track_id]["person_frames"] = 0
                    if track_history[track_id]["class_id"] == 0: 
                        track_history[track_id]["class_id"] = -1
                        
            if cls_id != 1:
                track_history[track_id]["falldown_miss"] += 1
                if track_history[track_id]["falldown_miss"] > 60:
                    track_history[track_id]["falldown_frames"] = 0
                    falldown_reset += 1
                    if track_history[track_id]["class_id"] == 1: 
                        track_history[track_id]["class_id"] = -1
                        
            if cls_id != 2:
                track_history[track_id]["attack_miss"] += 1
                if track_history[track_id]["attack_miss"] > 140:
                    track_history[track_id]["attack_frames"] = 0
                    attack_reset += 1
                    if track_history[track_id]["class_id"] == 2: 
                        track_history[track_id]["class_id"] = -1
                        
            if cls_id != 3:
                track_history[track_id]["smoke_miss"] += 1
                if track_history[track_id]["smoke_miss"] > 60:
                    track_history[track_id]["smoke_frames"] = 0
                    smoking_reset += 1
                    if track_history[track_id]["class_id"] == 3: 
                        track_history[track_id]["class_id"] = -1  
            
            crops.append({
                "track_id": track_id, 
                "class_id": track_history[track_id]["class_id"],
                "bbox_margin": (x1m, y1m, x2m, y2m), 
                "crop": frame[y1m:y2m, x1m:x2m].copy(),
                "falldown_frames": track_history[track_id]["falldown_frames"], 
                "attack_frames": track_history[track_id]["attack_frames"], 
                "smoke_frames": track_history[track_id]["smoke_frames"],
                "person_frames": track_history[track_id]["person_frames"],
                "falldown_reset": falldown_reset,
                "attack_reset": falldown_reset,
                "smoking_reset": smoking_reset
            })
    return crops