import numpy as np
import math

# 세 점(관절) 사이의 각도를 구하는 함수
def calculate_angle(a, b, c):
    a = np.array(a) # 끝
    b = np.array(b) # 가운데
    c = np.array(c) # 끝
    
    # atan2를 이용해 라디안 값 계산 후 각도(degree)로 변환
    radians = np.arctan2(c[1] - b[1], c[0] - b[0]) - np.arctan2(a[1] - b[1], a[0] - b[0])
    angle = np.abs(radians * 180.0 / np.pi)
    
    if angle > 180.0:
        angle = 360 - angle
        
    return angle

def calculate_distance(p1, p2):
    return math.sqrt((p1[0] - p2[0])**2 + (p1[1] - p2[1])**2)


def is_attack(landmarks):
    # 주요 관절 좌표 추출 (x, y)
    l_shoulder = [landmarks[11]['x'], landmarks[11]['y']]
    l_elbow = [landmarks[13]['x'], landmarks[13]['y']]
    l_wrist = [landmarks[15]['x'], landmarks[15]['y']]
    l_hip = [landmarks[23]['x'], landmarks[23]['y']]

    r_shoulder = [landmarks[12]['x'], landmarks[12]['y']]
    r_elbow = [landmarks[14]['x'], landmarks[14]['y']]
    r_wrist = [landmarks[16]['x'], landmarks[16]['y']]
    r_hip = [landmarks[24]['x'], landmarks[24]['y']]
    
    # 관절 각도 계산 
    # (어깨 - 팔꿈치 - 손목)
    l_elbow_angle = calculate_angle(l_shoulder, l_elbow, l_wrist)
    r_elbow_angle = calculate_angle(r_shoulder, r_elbow, r_wrist)
    # (골반 - 어깨 - 팔꿈치)
    l_shoulder_angle = calculate_angle(l_hip, l_shoulder, l_elbow)
    r_shoulder_angle = calculate_angle(r_hip, r_shoulder, r_elbow)

    # 거리 계산 (팔을 앞으로 뻗었는지)
    l_dist_x = abs(l_wrist[0] - l_shoulder[0])
    r_dist_x = abs(r_wrist[0] - r_shoulder[0])
    
    # 팔꿈치 각도가 펴져있고, 어깨 각도가 적절 하고(겨드랑이가 들려 있고), 손목이 골반보다 위에 있고 (0이 y가 높고 1이 y가 낮음), 일정 거리 이상 손이 뻣어있다
    is_l_attack = (l_elbow_angle > 130) and (45 < l_shoulder_angle < 160) and (l_wrist[1] < l_hip[1]) and (l_dist_x > 0.1)
    is_r_attack = (r_elbow_angle > 130) and (45 < r_shoulder_angle < 160) and (r_wrist[1] < r_hip[1]) and (r_dist_x > 0.1)    

    return is_l_attack or is_r_attack

def is_smoke(landmarks):
    smoke = False
    
    # 주요 관절 좌표 추출 (x, y)
    # 잎 위치
    mouth_l = [landmarks[9]['x'], landmarks[9]['y']]
    mouth_r = [landmarks[10]['x'], landmarks[10]['y']]
    mouth_center = [(mouth_l[0] + mouth_r[0]) / 2, (mouth_l[1] + mouth_r[1]) / 2]

    # 관절 위치
    l_shoulder = [landmarks[11]['x'], landmarks[11]['y']]
    l_elbow = [landmarks[13]['x'], landmarks[13]['y']]
    l_wrist = [landmarks[15]['x'], landmarks[15]['y']]
    
    r_shoulder = [landmarks[12]['x'], landmarks[12]['y']]
    r_elbow = [landmarks[14]['x'], landmarks[14]['y']]
    r_wrist = [landmarks[16]['x'], landmarks[16]['y']]

    # 어깨 위치
    shoulder_width = calculate_distance(l_shoulder, r_shoulder)
    if shoulder_width < 0.05: 
        shoulder_width = 0.1
        
    # 잎과 손목 거리
    dist_l_wrist_to_mouth = calculate_distance(mouth_center, l_wrist)
    dist_r_wrist_to_mouth = calculate_distance(mouth_center, r_wrist)
    
    # 손목이 굽혀져서 담배를 피는지 각도 계산
    l_elbow_angle = calculate_angle(l_shoulder, l_elbow, l_wrist)
    r_elbow_angle = calculate_angle(r_shoulder, r_elbow, r_wrist)

    # 판별식 어꺠 길이 기준으로 어꺠길이의 80%보다 작으면 가깝다
    is_l_close = dist_l_wrist_to_mouth < (shoulder_width * 0.8)
    is_r_close = dist_r_wrist_to_mouth < (shoulder_width * 0.8)
    
    is_l_smoking = is_l_close and (l_elbow_angle < 100)
    is_r_smoking = is_r_close and (r_elbow_angle < 100)
    
    # 최종 검증 통과 시
    if is_l_smoking or is_r_smoking:
        smoke = True
    return smoke

def is_falldown(landmarks):
    # 코의 y좌표
    nose_y = landmarks[0]['y']

    # 양쪽 골반(허리)의 평균 y좌표
    hip_y = (landmarks[23]['y'] + landmarks[24]['y']) / 2

    # 쓰러짐 판별 코가 허리보다 아래에 있을떄
    falldown = nose_y > hip_y
    
    return falldown