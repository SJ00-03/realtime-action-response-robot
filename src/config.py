import os

os.environ["QT_QPA_PLATFORM"] = "xcb" # OpenCV 경고창 끄기

# File Paths
HEF_PATH = "custom7_yolov11sio4.hef"  
MODEL_PATH = 'pose_landmarker_lite.task'

# Camera & Detection Settings
WEBCAM_ID = 0              
CONF_THRESH = 0.1         
CLASS_NAMES = {0: "person", 1: "falldown", 2: "attack", 3: "smoking"}

# Limits & Thresholds
LEFT_LIMIT = 0.22
RIGHT_LIMIT = 0.78
WEB_SEND_EVERY_N_FRAMES = 2
ROBOT_LEFT_LIMIT = 140
ROBOT_RIGHT_LIMIT= 500

# Debugging
WEBSITE_ON = False
USE_MULTIPROCESSING = True  
MAX_TEST_FRAMES = 5000      
DEBUG_MODE = True

POSE_CONNECTIONS = frozenset([(0, 1), (1, 2), (2, 3), (3, 7), (0, 4), (4, 5),
                              (5, 6), (6, 8), (9, 10), (11, 12), (11, 13),
                              (13, 15), (15, 17), (15, 19), (15, 21), (17, 19),
                              (12, 14), (14, 16), (16, 18), (16, 20), (16, 22),
                              (18, 20), (11, 23), (12, 24), (23, 24), (23, 25),
                              (24, 26), (25, 27), (26, 28), (27, 29), (28, 30),
                              (29, 31), (30, 32), (27, 31), (28, 32)])

# webserver
SERVER_URL = 'http://:5000/upload'
SERVER_COMMAND_URL = 'http://:5000/command'
JPEG_QUALITY = 75