# Philo — Raspberry Pi / Laptop Vision Stack

The high-level "brain" for Philo. Runs computer vision, decides how the robot
should move, and sends commands to the microcontroller (ESP32 on the real robot,
Arduino Uno or Pico for bench testing) over serial.

Currently runs on a **laptop with a webcam** for development. Moves to a
**Raspberry Pi 4 + Pi Camera v2** for the final robot — the code is the same,
only the camera source changes (`--webcam` flag).

## What it does

1. **Person following** — YOLOv8n-pose finds the nearest person; bounding-box
   center steers left/right, box size sets speed (bigger = closer = slower).
2. **Gesture stop** — raise a wrist above the shoulder for 5 frames to halt;
   lower it for 5 frames to resume.
3. **Object labels** — YOLOv8 tags non-person objects in view (apple, bottle…).
4. **ASL fingerspelling** — MediaPipe hand landmarks + a trained classifier
   recognize A–Z letters and send them to the OLED display.

## Files

| File | Role |
|------|------|
| `main.py` | Main loop — ties following, gesture stop, and ASL together |
| `tracker.py` | YOLOv8 pose + object detection wrapper |
| `controller.py` | Bounding box → motor commands + gesture state machine |
| `uart.py` | Serial link to the microcontroller (`CMD`/`STATUS`) |
| `asl_features.py` | **Canonical** scale-invariant hand-feature extraction (shared by training + inference) |
| `asl_from_dataset.py` | Train the ASL model from the Kaggle ASL Alphabet dataset |
| `asl_collect.py` | Alternative: collect your own ASL samples from webcam |
| `asl_run.py` | Standalone ASL tester (no following/gesture) |

## Serial protocol

```
Pi  -> MCU :  CMD <left> <right> <flags>\n
MCU -> Pi  :  STATUS <vbat> <dl> <dc> <dr> <el> <er> <ax> <ay> <az> <gz>\n   (10 Hz)
Pi  -> MCU :  ASL <LETTER>\n        # shows the letter on the OLED
```
115200 baud. On the Pi: `/dev/serial0`. On Windows bench testing: the Arduino/Pico COM port.

## Setup

```bash
pip install ultralytics opencv-python mediapipe scikit-learn pyserial
```

Model weights and the trained classifier are **not** committed (see `.gitignore`):
- `yolov8n-pose.pt` / `yolov8s.pt` — auto-download on first run
- `hand_landmarker.task` — auto-downloads on first run
- `asl_model.pkl` — regenerate it (see below)

## Train the ASL model

Download the [Kaggle ASL Alphabet dataset](https://www.kaggle.com/datasets/grassknoted/asl-alphabet),
extract it, then:

```bash
python asl_from_dataset.py --dataset "PATH/TO/asl_alphabet_train"
```

This extracts scale-invariant hand landmarks from the images and trains a small
neural network, saving `asl_model.pkl`.

## Run

```bash
# Laptop webcam, no hardware
python main.py --webcam

# Laptop webcam + Arduino/Pico on COM8
python main.py --webcam --serial COM8

# Raspberry Pi + ESP32
python main.py --serial /dev/serial0
```

Press **Q** in the display window to quit.
