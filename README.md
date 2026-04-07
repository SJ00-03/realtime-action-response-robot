# 실시간 행동 분석 기반 대응 로봇
### Real-time Behavior Analysis-based Response Robot


> 공공장소에서 사람과 이상행동을 실시간으로 감지하고, 이동형 순찰 로봇이 상황에 맞춰 추적·경고·상태 전송까지 수행하는 **온디바이스 AI 로봇 시스템**
>
> **STM32F407 · YOLO11s · Hailo-8 NPU ·  Raspberry Pi 5 · MediaPipe Pose**

AI융합 로봇 SW개발자 2기: 최우수상

---

## 📝 프로젝트 개요

- **주제**: 사람 및 이상행동(`person`, `falldown`, `attack`, `smoking`) 실시간 탐지 및 대응
- **형태**: 팀 프로젝트 / 온디바이스 AI + 임베디드 제어 + 로봇 시스템 통합
- **기간**: **2026.03.06 ~ 2026.03.20 (약 2주)**
- **목표**: 탐지, 추적, 대응하는 **실시간 순찰 로봇** 구현
- **주요 기능**
  - Raspberry Pi 5 + Hailo-8(NPU) 기반 **실시간 온디바이스 추론**
  - MediaPipe Pose 기반 **2차 자세 검증**으로 오탐 감소
  - STM32F407 기반 **주행/경보/외부장치 제어**
  - 웹 대시보드와 앱 연동을 통한 **실시간 모니터링 및 제어**

---
## 📋 목차
- [프로젝트 소개](#-프로젝트-소개)
- [시스템 아키텍처](#-시스템-아키텍처)
- [주요기능](#-주요기능)
- [탐지 클래스 및 로봇 동작](#-탐지-클래스-및-로봇-동작)
- [기술 스택](#-기술-스택)
- [실험 및 성능 평가](#-실험-및-성능-평가)
- [구현 내용 요약](#-구현-내용-요약)
- [트러블슈팅](#-트러블슈팅)
- [프로젝트 일정](#-프로젝트-일정)
- [담당 역할](#-담당-역할)



---
## 📌 프로젝트 소개


기존 CCTV 중심 관제 시스템은 고정된 시야, 사각지대, 관제 인력 의존, 그리고 탐지 이후 즉각적인 현장 대응이 어렵다.

이 프로젝트는 이러한 문제를 해결하기 위해, **이상행동을 실시간으로 감지하고 로봇이 직접 반응하는 이동형 AI 순찰 시스템**을 구현한 프로젝트입니다.

로봇은 평상시에는 **라인트레이싱 기반으로 순찰 주행**을 수행하고, 객체가 탐지되면 **추적 주행 모드**로 전환됩니다. 이후 객체의 상태를 분석해 `낙상`, `공격`, `흡연` 등 특정 상황으로 판단되면, 상황에 맞는 **LED / 부저 / 레이저 / 웹 전송** 동작을 수행합니다.

---

## 🏗️ 시스템 아키텍처

<div align="center">
    <img src="https://github.com/user-attachments/assets/4c1e567d-2a9b-4db8-a537-bf35897c9945" width="76%"> 
    <br>
    <p> 로봇 시스템 구성도 </p>
</div>
    
## 📌 로봇 시스템 구성도 (Robot System Architecture)

본 시스템은 다양한 센서와 AI 처리 장치를 기반으로 **상황 인식 및 실시간 대응이 가능한 자율 로봇 시스템**입니다.

### 🔹 입력 및 센서 (Input & Sensors)
- **Webcam**: 영상 데이터 입력 (객체 및 행동 인식)
- **TC RST5000 IR Sensor**: 라인 트래킹을 위한 경로 인식
- **Application + HC-06 Bluetooth Module**: 사용자 제어 및 데이터 송수신
- **Raspberry Pi 5 + Hailo-8 NPU**: 영상 기반 AI 연산 및 상황 판단

---

### 🔹 제어부 (Control Unit)
- **STM32F407 MCU**
  - Raspberry Pi로부터 전달받은 **위치 및 상황 데이터 처리**
  - 라인트레이싱 및 주행 제어
  - 각종 출력 장치 제어 (모터, 부저 등)

---

### 🔹 출력 및 액추에이터 (Output & Actuators)
- **L298N Motor Driver + DC Geared Motor**
  - 로봇 이동 및 방향 제어

- **Servo Motor**
  - 카메라 또는 센서 방향 제어

- **Buzzer Module / LED / Laser Module**
  - 경고 및 시각/청각 알림 제공

---

### 🔄 동작 흐름 (Workflow)
1. Webcam과 센서를 통해 환경 데이터 수집  
2. Raspberry Pi + Hailo에서 AI 기반 상황 분석 수행  
3. 분석 결과(위치, 행동 정보)를 STM32로 전달  
4. STM32가 주행 및 장치 제어 수행  
5. 이상 상황 발생 시 경고 및 알림 출력  

---

## 🚀 주요 특징 (Key Features)
- AI 기반 실시간 상황 인식 (영상 + 센서 융합)  
- MCU 기반 안정적인 주행 및 하드웨어 제어  
- 다양한 출력 장치를 통한 즉각적인 대응  
- 무선 통신(Bluetooth)을 통한 외부 제어 및 모니터링  
<br><br>

<div align="center">
<img src="https://raw.githubusercontent.com/gammapasta/temp/refs/heads/main/imgs/%EB%9D%BC%EC%A6%88%EB%B2%A0%EB%A6%AC%ED%8C%8C%EC%9D%B4_%EB%8B%A4%EC%9D%B4%EC%96%B4%EA%B7%B8%EB%9E%A8.png" width="80%" align="center"> 
    <p> 간단한 아키텍쳐 </p>
</div>
    
<details>
  <summary>자세한 아키텍쳐</summary>
    
  <img src="https://raw.githubusercontent.com/gammapasta/temp/refs/heads/main/imgs/%EB%9D%BC%EC%A6%88%EB%B2%A0%EB%A6%AC%ED%8C%8C_%EC%9E%90%EC%84%B8%ED%95%9C_%EB%8B%A4%EC%9D%B4%EC%96%B4%EA%B7%B8%EB%9E%A8.png" width="80%"> 


</details>
    
## 📌 시스템 구조 (System Architecture)

본 시스템은 **이상 행동(폭행, 낙상, 흡연)을 실시간으로 탐지하고 대응하는 엣지 기반 AI 감시 로봇**입니다.

### 🔹 입력 (Input)
- Webcam을 통해 실시간 영상 데이터 수집

### 🔹 엣지 컴퓨팅 (Raspberry Pi + Hailo NPU)
- Core 0 (Main): 전체 시스템 제어 및 화면 출력  
- Core 1 (YOLOv11s): 객체 및 이상 행동 탐지 (Person, Attack, Smoking, Falldown 등)  
- Core 2 (MediaPipe Pose): 사람의 자세 분석 (낙상 판단, 흡연 등)  
- Core 3 (Server/Web): 외부 서버 및 웹 통신  

### 🔹 출력 (Output)
- MCU (STM32, GPIO): 부저, 모터 등 하드웨어 제어  
- Web (Wi-Fi): 알림 및 모니터링 데이터 전송  

## 🔄 동작 흐름 (Workflow)
1. Webcam으로 영상 입력  
2. YOLO를 통한 객체 및 행동 탐지  
3. MediaPipe를 통한 자세 분석  
4. 상황 판단 (폭행, 낙상, 흡연)  
5. 경고, 추적, 알림 등 대응 수행  

## 🚀 주요 특징 (Key Features)
- 온디바이스 기반 실시간 AI 처리  
- YOLO + Pose 기반 멀티 모델 분석  
- 탐지부터 대응까지 자동화된 시스템
  
<br><br>


---

## ✨ 주요 기능

### 1) 실시간 온디바이스 AI 추론
- CPU 기반 실행 대비 **약 26배 빠른 추론 속도**
- **43.9 FPS**, **18.8 ms latency**로 실시간 처리 가능 수준 확보
- Hailo-8 NPU 배포용 **HEF 모델** 최적화 완료
    
### 2) 실시간 행동 분석
- `person`, `falldown`, `attack`, `smoking` 4개 클래스를 탐지
- YOLO11s + ByteTrack으로 객체 검출 및 추적
- MediaPipe Pose로 33개 랜드마크를 추출해 자세 기반 재판별 수행
    
### 3) AI + 임베디드 이중 제어 구조 설계
- **Raspberry Pi 5**: 비전 처리, 객체 추적, 행동 판별, 웹 전송
- **STM32F407**: 모터, LED, 부저, 레이저, 라인트레이싱 등 실시간 하드웨어 제어
- 두 시스템 간 **8-Bit GPIO 병렬 통신 프로토콜** 구현

### 4) 정확도와 안정성을 고려한 구조
- YOLO11s 단독 탐지 이후, **MediaPipe Pose 기반 2차 검증** 수행
- `attack` / `smoking`처럼 혼동되기 쉬운 클래스의 오탐을 낮추기 위해
  **관절 좌표 + 기하학적 규칙**을 결합한 판단 로직 적용
- track_id 기준 **의심 단계 / 확정 단계** 누적 판별로 순간 오탐 경보 방지
    
### 5) 단계적 대응 시나리오
- **의심 단계**: LED 점멸, 추적 또는 접근 시작
- **확정 단계**: LED 점등, 부저/레이저 활성화, 상태 정보 웹 전송
    
### 6) 순찰 및 추적 주행
- 평상시에는 **라인트레이싱 기반 순찰 주행**
- 객체 탐지 시 **위치 기반 추적 주행 모드** 전환
- 상황 종료 후 초기 위치 복귀를 고려한 제어 구조 설계
    
### 7) 관리자 모니터링
- 실시간 영상 스트리밍
- 클래스별 감지 건수 시각화
- 상태 수신 시각, 프레임 수신 시각, FPS 등 대시보드 제공

### 8) 앱 연동 수동 제어
- 스마트폰 앱과 Bluetooth UART 통신
- 수동 주행, 정지, 속도 조절, 비상 동작 등 제어 가능
- 좌, 우 각 모터 속도 조절로 수동 휠 얼라이먼트 기능 구현


---

## <img src="http://10.10.14.29:3000/uploads/184bc6cf-bfd3-49de-bd87-2ab1778ecb7c.png" style="width: 40px; height: auto; object-fit: contain; vertical-align: middle;" alt="자율주행 로봇"> <span style="vertical-align: middle;">탐지 클래스 및 로봇</span>

| 클래스 | 판단 기준 요약 | 로봇 동작 |
|---|---|---|
| `person` | 일반 사람 탐지 및 일정 프레임 이상 유지 | 녹색 LED, 위치 기반 추적 주행 |
| `falldown` | 박스 비율, 몸통 수평각, 어깨-골반 y축 차이 등 | 청색 LED + 부저, 대상 접근, 상태 웹 전송 |
| `smoking` | 손목-입 거리, 팔꿈치 각도 등 | 황색 LED + 부저, 대상 접근, 상태 웹 전송 |
| `attack` | 팔꿈치 각도, 손목 높이, 수평 이동량 등 | 적색 LED + 부저 + 레이저, 위험 인물 추적, 상태 웹 전송 |

---



## 💻 기술 스택


| 구분 | 기술 |
|---|---|
| AI / Vision | YOLO11s, Hailo-8 NPU, MediaPipe Pose, ByteTrack, OpenCV |
| Embedded | STM32F407, Bare-metal C, TIMER, UART, PWM, GPIO, Interrupt |
| Edge Device | Raspberry Pi 5, Raspberry Pi AI HAT+ |
| Backend / Monitoring | Flask, HTTP POST, Web Dashboard |
| App | MIT App Inventor, Bluetooth UART |
| Language | Python, C |
| Hardware | L298N, DC Geared Motor, Servo Motor, TCRT5000, HC-06, USB Webcam |

---

## 📊 실험 및 성능 평가

### 모델 학습 성능

| 항목 | 결과 |
|---|---:|
| Final Model mAP@50-95 | **0.898** |
| 전체 mAP@50 | **0.977** |
| Precision | **0.914** |
| Recall | **0.977** |
    

<img src="https://raw.githubusercontent.com/gammapasta/temp/refs/heads/main/imgs/%EC%A4%91%EA%B0%84%ED%81%AC%EA%B8%B0___learning_curve_mAP50_95.png"  width="80%">
    
초기부터 마지막 학습까지의 성능 비교표 (run1 - run7)


본 모델은 YOLO 기반 객체 탐지 모델로 학습되었으며, 다양한 성능 지표를 통해 학습 결과를 평가하였다.

mAP@50-95 기준 약 **89.8%의 높은 정확도**를 보이며  mAP@50에서는 **97.7%로 매우 우수한 탐지 성능**을 확인할 수 있다.  

또한 Precision과 Recall 모두 높은 값을 보여,  **정확하게 탐지하면서 (Precision)**  **놓치는 경우도 매우 적은 (Recall)**  균형 잡힌 성능을 달성하였다.

---

### 🔹 학습 곡선 분석 (Learning Curve)
그래프는 각 학습(run1 ~ run7)과 최종 모델의 mAP 변화를 나타낸다.

- 초기 epoch에서 성능이 빠르게 상승하며 **모델이 빠르게 수렴**하는 경향을 보인다.  
- 이후 완만한 상승을 통해 점진적으로 성능이 향상된다.  
- 최종 모델(Final Model)은 전체 실험 중 **가장 높은 성능 곡선**을 유지한다.  

👉 이는 데이터셋과 학습 전략이 효과적으로 적용되었음을 의미한다.

---

### 🔹 종합 평가 (Summary)
- 빠른 수렴 속도와 안정적인 학습 과정  
- 높은 mAP, Precision, Recall을 기반으로 한 신뢰성 있는 탐지 성능  
- 실제 환경에서도 적용 가능한 수준의 성능 확보  

👉 따라서 본 모델은 **실시간 이상 행동 탐지 시스템에 적합한 성능을 갖는다.**
    
    
### 온디바이스 모델 성능

| 항목 | CPU 기반 (.pt) | Hailo HEF |
|---|---:|---:|
| 추론 속도 (FPS) | 약 1.7 FPS | **약 43.9 FPS** |
| 지연 시간 | 600.9 ms | **18.8 ms** |
| MAP@50:95 | 0.822 | **0.813** |
| 실시간 처리 가능 여부 | 불가능 | **가능** |



> 양자화(INT8)와 배포 최적화 과정에서 정확도 손실을 최소화하면서, 엣지 디바이스 환경에서 실시간 처리 가능한 수준으로 최적화 진행.



    



<table>
  <tr>
    <td><img src="https://raw.githubusercontent.com/gammapasta/temp/refs/heads/main/imgs/%EC%A4%91%EA%B0%84%ED%81%AC%EA%B8%B0___performance_comparison.png"  width="100%"></td>
    <td><img src="https://raw.githubusercontent.com/gammapasta/temp/refs/heads/main/imgs/%EC%A4%91%EA%B0%84%ED%81%AC%EA%B8%B0___Latency_comparison.png"  width="100%">
    </td>
    <td><img src="https://raw.githubusercontent.com/gammapasta/temp/refs/heads/main/imgs/%EC%A4%91%EA%B0%84%ED%81%AC%EA%B8%B0figure_trade_off.png"  width="100%"></td>
  </tr>
<tr>
     <td>
        정확도 MAP@50 95 비교
    </td>
    <td>
        레이턴시 비교
    </td>
    <td>
        정확도 FPS 비교
    </td>
</tr>
</table>
    
<details>
    
  <summary>평가 과정</summary>
    
    
1) .hef 모델 (Hailo-8) 평가
``` 
hailo eval --model hailo_model.hef --dataset-path /hailo/dataset --json-path /hailo/dataset/annotations.json
```
2) .pt 모델(원본) 평가
```
yolo task=detect mode=val model=finalModel.pt data=dataset.yaml split=test device=cpu
```

</details>
본 실험은 동일한 모델을 **CPU 기반(.pt)**과 **Hailo NPU(HEF)** 환경에서 비교하여  
엣지 디바이스에서의 실시간 처리 성능을 평가하였다.

👉 Hailo NPU 적용 시  
- **약 25배 이상의 속도 향상**  
- **지연 시간 대폭 감소 (600ms → 18ms)**  
- 정확도는 거의 유지  

즉, **정확도 손실을 최소화하면서 실시간 처리 성능 확보**에 성공하였다.


---

## 온디바이스 포멧으로 변환 과정


### 1. WSL2.0 설치 

### 2. WSL2.0 내부에 도커 설치
- 도커설치 → hailo sw 설치하기 → shared_with_docker 이동
    
### 3. 도커 실행
- 터미널에서 ./hailo_ai_sw_suite_docker_run.sh --override 실행 후 shared 폴더로 이동

### 4. 도커 내부에서 GPU 작동 하는지 확인
 - 명령어 입력 후 GPU가 잡히는지 확인
```
export CUDA_VISIBLE_DEVICES=0
export TF_FORCE_GPU_ALLOW_GROWTH=true
```

#6. hef 파일로 변환

```
hailomz compile --yaml custom_yolov11s.yaml --hw-arch hailo8 --ckpt test17best.onnx --calib-path /local/shared_with_docker/calibration_dataset_letterbox --classes 4
```



---

## 🔍 구현 내용 요약

### 1. 2단계 행동 판별 구조
YOLO 탐지 결과 + MediaPipe Pose 분석 결과를 결합해 최종 판단 수행. 이를 통해 오탐 억제 강화.

### 2. 멀티프로세스 기반 파이프라인
단일 스레드에서 HEF 추론과 자세 분석을 함께 수행할 경우 FPS가 크게 저하되는 문제를 해결하기 위해,
프레임 획득 및 계산 / 추론 / 포즈 분석 / 하드웨어 I/O를 멀티프로세스 구조로 분리.

- 멀티프로세싱, 멀티스레딩 비교
<img src="https://raw.githubusercontent.com/gammapasta/temp/refs/heads/main/imgs/5000%EB%B2%88%ED%95%9C%EA%B1%B0.png"  width="60%">

### 3. STM32 Bare-metal 제어
HAL 라이브러리 없이 레지스터를 직접 제어하는 Bare-metal C 구조 적용. 이를 통해 예측 가능한 실시간 동작과 제어 지연 최소화.

### 4.하드웨어 타이머 및 인터럽트를 활용한 멀티태스킹
지연(Delay) 함수 대신 하드웨어 타이머(TIM3) 인터럽트를 활용. 이를 통해 메인 루프 방해 없는 실시간 병렬 제어(Non-blocking) 구현.

### 5.하드웨어 타이머 기반 정밀 PWM 모터 제어 및 보정
하드웨어 타이머(TIM4) 기반 20kHz 고주파 PWM 생성 및 좌우 오프셋 보정 로직 적용. 이를 통해 정밀한 모터 구동과 주행 직진성 향상.

### 6. 통신 신뢰성 개선
NPU 장착 시 발생하는 I2C 통신 간섭 문제를 8-Bit GPIO 병렬 통신으로 전면 전환하여 해결. 이를 통해 하드웨어 간섭 원천 차단 및 통신 안정성 확보.

---

## ⚠️ 트러블슈팅

| 문제 | 원인 | 해결 방법 | 결과 |
|---|---|---|---|
| 공격 / 흡연 클래스 혼동 | 손 위치가 얼굴 근처에 있어 클래스 특징이 유사 | 혼동 샘플 선별, 라벨 재검수, 데이터 정제 | 클래스 간 독립성 향상 |
| HEF 변환 후 객체 미탐지 | 비율 유지 없는 강제 리사이즈로 객체 왜곡 | letterbox 전처리 및 좌표 복원 적용 | 객체 탐지 정상화 |
| I2C 통신 장애 | Hailo AI HAT 사용 시 버스 간섭 발생 | 8-Bit GPIO 병렬 통신으로 전환 | 통신 신뢰성 확보 |
| Raspberry Pi 전원 꺼짐 | 순간 전류 부족으로 저전압 경고 발생 | 고방전 LiPo 배터리팩 적용 | 전원 안정성 확보 |

---

## 📅 프로젝트 일정

> 요구분석부터 최종 검증까지 약 2주간 병렬 개발 방식으로 진행했습니다.

| 기간 | 주요 작업 |
|---|---|
| 3/6 ~ 3/8 | 요구분석 및 설계, 로봇 HW 인터페이스 설계, 데이터 전처리 시작 |
| 3/9 ~ 3/11 | 기구부 설계 및 프로토타입 제작, 모델 학습 시작(run1~run3), Raspberry Pi 5 프로그램 개발 시작 |
| 3/12 ~ 3/13 | 로봇 주행 시스템 개발, 라인트레이싱 순찰주행 개발, MCU 주변장치 제어 로직 구현, 데이터 전처리 및 모델 학습(run4~run5) |
| 3/14 ~ 3/15 | MCU-휴대폰 UART 통신, MCU-RPi 병렬통신, RPi 전원공급 시스템 개발, 모델 학습(run6~run7) |
| 3/16 ~ 3/17 | 시스템 통합, HEF 모델 변환, 제어 파라미터 튜닝 및 최적화, Final Model 학습 |
| 3/18 ~ 3/20 | 동작 테스트 및 최종 검증, 영상 촬영, 보고서 작성, 최종 제출 준비 |

---


## 📂 리포지토리 구성

```bash
.
├── README.md
├── raspberry_pi/
│   └── utils/
│       ├ __init__.py
│       ├ rapi_logics.py
│       └ utils.py
│   └── workers/
│       ├ hailo_worker.py
│       ├ hardware_worker.py
│       └ mediapipe_worker.py
│   └── config.py
│   └── main.py
│    
├── stm32/
│   └── vision_Ai_robot_Guide.c
│
├── app/
│   └── inventor/
├── docs/
│   ├── circuit/
│   ├── report/
│   └── images/
└── assets/
    ├── demo.gif
    └── architecture.png
```


---
## 담당 역할



## @@@@@@@@@@@@@@ 지우시오
## @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
## 🧑‍💻 담당 역할
```
> **이 섹션은 취업용 포트폴리오에서 가장 중요합니다.**  
> 아래 예시를 참고해서 **반드시 본인 기여 중심으로 수정**하세요.

예시)
- 데이터 수집 및 라벨 정제, 클래스 혼동 샘플 클리닝
- YOLO11s 파인튜닝 및 Hailo HEF 배포 최적화
- Raspberry Pi 멀티프로세싱 파이프라인 구현
- MediaPipe Pose 기반 2차 판별 로직 설계
- STM32와 Raspberry Pi 간 8-Bit GPIO 통신 설계
- STM32 Bare-metal 기반 모터 / LED / 부저 / 레이저 제어 구현
- 웹 대시보드 또는 앱 연동 기능 구현

**작성 팁**
- `무엇을 했다`보다 **`어떤 문제를 해결했고 어떤 결과를 냈는지`** 중심으로 적는 것이 좋습니다.
- 가능하면 수치(FPS, latency, mAP, 통신 안정화, 오탐 감소 등)를 함께 적으세요.

예시 문장)
- Hailo 배포 파이프라인에서 letterbox 누락으로 발생하던 객체 미탐지 문제를 해결해, HEF 모델이 실환경에서 정상 동작하도록 개선했습니다.
- Raspberry Pi와 STM32 간 통신 구조를 I2C에서 8-Bit GPIO 병렬 통신으로 재설계하여 하드웨어 간섭 문제를 해결했습
    니다.
- YOLO11s와 MediaPipe Pose를 결합한 2단계 판별 로직을 적용해 행동 클래스 간 오탐을 줄였습니다.
```

--- 


## 📑 첨부자료


- 시연 영상: https://www.youtube.com/watch?v=8uklY-u2ZY4&feature=youtu.be
- 회로도 이미지
- 모델 성능 그래프
- 웹 대시보드 캡처
- 실제 주행 / 추적 장면 이미지
- 본인 기여 상세 문서 (`docs/contribution.md` 등)

---



