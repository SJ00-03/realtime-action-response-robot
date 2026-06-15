# 🤖 실시간 행동 분석 기반 AI 안전관리 로봇

## 📌 프로젝트 핵심

> 실시간 영상에서 **낙상·흡연·공격 행동**을 감지하고, **웹 모니터링 UI**와 **Bluetooth 로봇 제어**에 연동한 AI 안전관리 로봇입니다.

**My Role**: Dataset 전처리 · YOLO 학습 · Web Dashboard · Bluetooth 제어 연동  
**Key Result**: 약 4만 장 커스텀 데이터셋 / **mAP@50 0.977** / **43.9 FPS**

![Python](https://img.shields.io/badge/Python-3776AB?style=flat-square&logo=python&logoColor=white)
![YOLO](https://img.shields.io/badge/YOLO-111F68?style=flat-square)
![OpenCV](https://img.shields.io/badge/OpenCV-5C3EE8?style=flat-square&logo=opencv&logoColor=white)
![MediaPipe](https://img.shields.io/badge/MediaPipe-0097A7?style=flat-square)
![Raspberry Pi](https://img.shields.io/badge/Raspberry%20Pi-A22846?style=flat-square&logo=raspberrypi&logoColor=white)
![STM32](https://img.shields.io/badge/STM32-03234B?style=flat-square)
![Bluetooth](https://img.shields.io/badge/Bluetooth-0082FC?style=flat-square&logo=bluetooth&logoColor=white)

**Demo media placeholders. Add the files to `assets/` when ready.**

---

## 📊 Results

| mAP@50 | mAP@50:95 | FPS | Latency |
|---:|---:|---:|---:|
| **0.977** | **0.898** | **43.9** | **18.8 ms** |

---

## 🎥 Demo

| 🎥 AI Detection | 🖥 Web Dashboard | 📡 Bluetooth Control |
|---|---|---|
| ![Detection Demo](assets/detection_demo.gif) | ![Web Dashboard](assets/web_dashboard.png) | ![Bluetooth Control](assets/bluetooth_control.png) |
| 사람·낙상·공격·흡연 감지 | 실시간 영상·FPS·알림·통계 | HC-06 + STM32 UART 수동 제어 |

---

## 🙋 My Contribution

| Area | What I did |
|---|---|
| **Dataset** | AI Hub + 직접 수집 데이터 정리, 4개 클래스 라벨링·검수 |
| **AI Training** | YOLO 커스텀 학습, Precision/Recall/mAP 분석, 혼동 클래스 재학습 |
| **Web Dashboard** | 실시간 영상, FPS, 위험 알림, 클래스별 상태·통계 UI 구현 |


> 팀 전체 구현 영역인 STM32 전체 제어, Hailo 변환, 하드웨어 제작은 별도 시스템 구성으로 구분했습니다.

---

## 📌 프로젝트 개요

CCTV 관제의 사각지대와 대응 지연을 줄이기 위한 **실시간 행동 분석 기반 AI 안전관리 로봇**입니다.  
감지 클래스는 `person`, `falldown`, `attack`, `smoking`입니다.

---

## 🏗 시스템 구조

```mermaid
flowchart LR
    A[Camera] --> B[Raspberry Pi 5]
    B --> C[YOLO / MediaPipe]
    C --> D[Web Dashboard]
    C --> E[STM32]
    E --> F[Motor / LED / Buzzer]
    G[Bluetooth App] --> E
```

---

## 🧰 Tech Stack & Details

| Category | Stack |
|---|---|
| AI / Vision | YOLO, MediaPipe Pose, OpenCV, ByteTrack, Hailo AI Accelerator |
| Edge / Embedded | Raspberry Pi 5, STM32F407, GPIO, UART, PWM, HC-06 Bluetooth |
| Web | HTML, CSS, JavaScript |
| Structure | `src/`, `sever/`, `stm32/`, `utils/`, `docs/`, `assets/` |

## 🔗 상세 문서

> 프로젝트의 세부 구현 내용은 아래 문서로 분리했습니다.

<table>
  <tr>
    <td align="center" width="25%">
      <b>🙋 담당 역할 상세</b><br/>
      데이터셋 · AI 학습 · 웹 · Bluetooth<br/><br/>
      <a href="./docs/contribution.md">문서 보기</a>
    </td>
    <td align="center" width="25%">
      <b>📘 기술 보고서</b><br/>
      전체 시스템 구조 및 구현 흐름<br/><br/>
      <a href="./docs/technical_report.md">문서 보기</a>
    </td>
    <td align="center" width="25%">
      <b>🚧 트러블슈팅</b><br/>
      문제 원인 · 해결 과정 · 결과<br/><br/>
      <a href="./docs/troubleshooting.md">문서 보기</a>
    </td>
    <td align="center" width="25%">
      <b>📈 학습 결과</b><br/>
      데이터셋 · mAP · 클래스별 성능<br/><br/>
      <a href="./docs/training_result.md">문서 보기</a>
    </td>
  </tr>
</table>
