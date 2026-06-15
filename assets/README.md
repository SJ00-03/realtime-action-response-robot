# Assets

이 폴더는 README와 문서에 사용할 이미지, GIF, 미디어 자료를 관리하는 위치입니다.

현재 이 폴더에는 README에서 참조할 수 있는 로컬 이미지/GIF 파일이 없습니다. 새 미디어를 추가할 때는 실제 파일명을 확인한 뒤 README에서 같은 경로를 사용해 깨진 링크가 생기지 않도록 합니다.

| File | Purpose |
|---|---|
| YouTube Link: https://youtu.be/Yur_1fwSsXE | Full integrated operation demo video |

## Recommended filenames for future assets

아래 파일명은 향후 미디어를 추가할 때 권장하는 이름입니다. 실제 파일이 추가되기 전에는 README에서 로컬 이미지 링크로 사용하지 않습니다.

| File | Purpose |
|---|---|
| `assets/cover_robot.png` | Project representative image |
| `assets/detection_demo.gif` | AI detection demo GIF |
| `assets/web_dashboard.png` | Web monitoring dashboard screenshot |
| `assets/datasets.png` | Dataset distribution or labeling result image |
| `assets/operation_flow.png` | System operation flow diagram |
| `assets/hardware_circuit.png` | Hardware circuit diagram |
| `assets/training_result.png` | YOLO training result graph |
| `assets/model_performance_comparison.png` | Model mAP performance comparison |
| `assets/latency_comparison.png` | Inference latency comparison |
| `assets/processing_performance.png` | Multi-processing and multi-threading performance comparison |
| `assets/final_performance_comparison.png` | Final model performance comparison |

## Checklist before linking assets

1. 파일이 실제로 `assets/` 폴더에 존재하는지 확인합니다.
2. README에 적은 경로와 실제 파일명이 대소문자까지 정확히 일치하는지 확인합니다.
3. 이미지, GIF, 영상 같은 바이너리 파일을 새로 추가하는 PR인지 먼저 확인하고, 필요한 경우 별도 PR로 분리합니다.
4. YouTube 영상은 GitHub README에서 `iframe` 대신 Markdown 썸네일 링크를 사용합니다.
