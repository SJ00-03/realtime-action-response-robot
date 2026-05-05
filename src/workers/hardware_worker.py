import time
import queue
import requests
import threading
import cv2
from gpiozero import LED
import config

def send_commands_to_webserver(SERVER_COMMAND_URL, situation_types):
    status_data = {"situation": situation_types}
    print(f"situation_types {situation_types} ")
    try:
        response = requests.post(SERVER_COMMAND_URL, json=status_data, timeout=0.5)
        if response.status_code == 200:
            print(f"send status success: {status_data}")
        else:
            print(f"response error: {response.status_code}")
    except requests.exceptions.RequestException as e:
        pass 
     
def set_pins(pins_list, value, bit_count):
    for i in range(bit_count):
        if (value >> i) & 1: pins_list[i].on()
        else: pins_list[i].off() 
    
def send_to_stm32(drive_pins, state_pins, drive_cmd, state_cmd, alive_pin, is_alive=True):
    set_pins(drive_pins, drive_cmd, 3)
    set_pins(state_pins, state_cmd, 4)
    if is_alive: alive_pin.on()
    else: alive_pin.off()
    print(f"send_to_stm32: {drive_cmd, state_cmd}")

def get_robot_move_command_final(box_center_x, box_height, state_cmd):
    if state_cmd == 0: return 0
    distance = 0; direction = 0

    if box_center_x < config.ROBOT_LEFT_LIMIT: direction = 3 
    elif box_center_x > config.ROBOT_RIGHT_LIMIT: direction = 4 
    else: direction = 0 

    if state_cmd == 3 or state_cmd == 4:
        if box_height < 240: distance = 1 
        elif box_height > 350: distance = 2 
        else: distance = 0
    else:
        if box_height < 310: distance = 1 
        elif box_height > 430: distance = 2 
        else: distance = 0 

    return direction if direction != 0 else distance




def hardware_worker(servo_command_queue, web_queue, web_video_queue):
    ''' stm보드로 명령, web 접속용 프로세스 '''
    print("[servo_web_stm Worker]  initailizing...")
    time.sleep(5)
    drive_pins = [LED(17), LED(27), LED(22)]
    state_pins = [LED(16), LED(19), LED(20), LED(21)]
    alive_pin = LED(26)  
    STATE_MAP = {"person": (1, 2), "falldown": (3, 4), "smoking": (5, 6), "attack": (7, 8)}
    print("[servo_web_stm Worker]  start")

    def servo_task():
        print("[Thread: Servo] start")
        while True:
            try:
                servo_command_msg = servo_command_queue.get() 
                if servo_command_msg is None: 
                    break 
                while True:
                    try: servo_command_msg = servo_command_queue.get_nowait()
                    except queue.Empty: 
                        break
                    
                detected_action = servo_command_msg["command"]
                trackings = servo_command_msg["trackings"]
                
                if not trackings:
                    for _ in range(5):
                        send_to_stm32(drive_pins, state_pins, 0, 0, alive_pin, is_alive=True)
                        time.sleep(0.2)
                    continue
                
                is_attack = False # 공격일경우 공격 상황을 우선으로 성정해서 쓰러짐이 포함 되더라도 공격하는 객체 찾기
                track_ids = []
                
                for track_id, track_data in trackings.items():
                    class_name = track_data["class_name"]
                    track_ids.append(track_id)
                    if class_name == "attack": 
                        is_attack = True
                        
                track_ids.sort()
                x1, y1, x2, y2 = trackings[track_ids[0]]["bbox"]
                cx = int((x1 + x2) / 2)
                bh = y2 - y1
                
                robot_state = 0
                
                # 공격은 2명 이상이라 공격하는사람 기준으로 명령 보내기
                if is_attack:
                    # 공격 의심
                    if detected_action["command_attack_warn"] > 0:
                        robot_state = STATE_MAP["attack"][0]
                        # 공격 확정
                        if detected_action["command_attack"] > 0: 
                            robot_state = STATE_MAP["attack"][1]
                else:
                    # 사람 의심
                    if detected_action["command_person_warn"] > 0:
                        robot_state = STATE_MAP["person"][0]
                        #사람 확정
                        if detected_action["command_person"] > 0: 
                            robot_state = STATE_MAP["person"][1]
                            
                    # 쓰러짐 의심
                    if detected_action["command_falldown_warn"] > 0:
                        robot_state = STATE_MAP["falldown"][0]
                        # 쓰러짐 확정
                        if detected_action["command_falldown"] > 0: 
                            robot_state = STATE_MAP["falldown"][1]
                            
                    # 담배 의심
                    if detected_action["command_smoking_warn"] > 0:
                        robot_state = STATE_MAP["smoking"][0]
                        # 담배 확정
                        if detected_action["command_smoking"] > 0: 
                            robot_state = STATE_MAP["smoking"][1]

                
                robot_move = get_robot_move_command_final(cx, bh, robot_state) # stm보드로 명령
                print(f" [robot_move] {robot_move}   [robot_state] {robot_state}")
                
                for _ in range(5):
                    send_to_stm32(drive_pins, state_pins, robot_move, robot_state, alive_pin, is_alive=True)
                    time.sleep(0.2)
                    
            except queue.Empty: continue           
            except Exception as e: print("[ERROR: Servo Thread]", e)

    def web_command_task():
        print("[Thread: Web Command] start")
        while True:
            try:
                latest_msg = web_queue.get()
                if latest_msg is None: 
                    break
                while True:
                    try: latest_msg = web_queue.get_nowait()
                    except queue.Empty: 
                        break
                    
                send_commands_to_webserver(config.SERVER_COMMAND_URL, latest_msg["situation_types"])
                time.sleep(1) # 1초마다 서버로 보냄
                
            except queue.Empty: continue
            except Exception as e: print("ERROR: Web Command Thread ", e)
        
    def web_video_task():
        print("[Thread: Web Stream] start")
        while True:
            try:
                latest_msg = web_video_queue.get()
                if latest_msg is None: 
                    break
                while True:
                    try: latest_msg = web_video_queue.get_nowait()
                    except queue.Empty: 
                        break
                
                # 웹서버로 프레임 보내기 전 최적화진행
                ok, img_encoded = cv2.imencode(".jpg", latest_msg["image"], [int(cv2.IMWRITE_JPEG_QUALITY), config.JPEG_QUALITY])
                if ok:
                    try:
                        response = requests.post(config.SERVER_URL, data=img_encoded.tobytes(), headers={'Content-Type': 'application/octet-stream'}, timeout=0.5) 
                        if response.status_code != 200: 
                            pass
                    except requests.exceptions.RequestException as e:
                        time.sleep(0.2) 
            except queue.Empty: continue
            except Exception as e: print("ERROR: Web Thread ", e)

    # 쓰레드 시작
    t1 = threading.Thread(target=servo_task, daemon=True)
    if config.WEBSITE_ON:
        t2 = threading.Thread(target=web_video_task, daemon=True)
        t3 = threading.Thread(target=web_command_task, daemon=True) 
    
    t1.start()
    if config.WEBSITE_ON:    
        t2.start(); t3.start()
        t2.join(); t3.join()
    t1.join()