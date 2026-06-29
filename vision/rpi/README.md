# Philo — Vision & Control Stack

The high-level "brain" for Philo. Runs computer vision, decides how the robot
should move, and sends commands to the microcontroller (Arduino Uno as the ESP32
stand-in on the bench; ESP32 on the final PCB) over serial.

**Runs on a Raspberry Pi 5** (the robot's brain) *or* a laptop (for development
and for ASL, which needs MediaPipe). Same code on both — only the camera source
and serial port differ.

## What it does

1. **Person following** — YOLOv8n-pose finds the nearest person; bounding-box
   center steers left/right, box size sets speed (bigger = closer = slower).
2. **Gesture stop** — raise a wrist above the shoulder for 5 frames to halt;
   lower it for 5 frames to resume.
3. **ASL fingerspelling** — MediaPipe hand landmarks + a trained classifier
   recognize A–Z letters and flash them on the OLED. *(Laptop, or Pi with the
   Python 3.12 env — see below.)*
4. **Manual drive** — an Xbox controller drives the robot; tap **A** to toggle
   between AUTO (follow) and MANUAL (you drive). `--manual`.
5. **Obstacle override** — runs on the Arduino itself: if anything is within
   20 cm it forces a STOP, even in manual or while following. Safety doesn't
   depend on the link staying up.
6. *(Optional)* **Object labels** — YOLOv8 tags non-person objects. Off by
   default (heavy); enable with `--objects` on a laptop.

## Files

| File | Role |
|------|------|
| `main.py` | Main loop — following, gesture, ASL, manual, display |
| `tracker.py` | YOLOv8 pose (object model optional, off by default for Pi speed) |
| `controller.py` | Bounding box → `CMD` + gesture state machine |
| `uart.py` | Serial link to the microcontroller (`CMD` out, `STATUS` in) |
| `gamepad.py` | Xbox controller → differential drive (pygame) |
| `asl_features.py` | **Canonical** scale-invariant hand features (shared by train + infer) |
| `asl_from_dataset.py` | Train ASL model: `--dataset` (extract+train) or `--csv` (train from features) |
| `asl_collect.py` | Alternative: collect your own ASL samples from webcam |
| `asl_run.py` | Standalone ASL tester |
| `webcam_stream.py` | Laptop → MJPEG stream (lets the Pi use a remote camera via `--source`) |
| `test_link.py` | Pi→Arduino link check (follow/turn/stop, no camera) |
| `gamepad_test.py` | Print controller axes/buttons for mapping |

## Serial protocol

```
Pi  -> MCU :  CMD <left> <right> <flags>\n      # wheel speeds -255..255; flags bit0=coast,bit1=brake
MCU -> Pi  :  STATUS <vbat> <dl> <dc> <dr> <el> <er> <ax> <ay> <az> <gz>\n   (10 Hz)
Pi  -> MCU :  ASL <LETTER>\n                    # shows the letter on the OLED
```
115200 baud. Pi → Arduino over USB = `/dev/ttyACM0`. On the final robot, Pi → ESP32 = `/dev/serial0`. Windows bench = `COM#`.

## Running it

**On the Pi (the brain)** — USB webcam + Arduino on `/dev/ttyACM0`, ASL via the 3.12 env:
```bash
cd ~/Philobot/vision/rpi
DISPLAY=:0 ~/philo312/bin/python main.py --webcam --serial /dev/ttyACM0
```
Add `--manual` for Xbox-controller drive. `DISPLAY=:0` shows the detection window on the Pi's monitor.

**On a laptop** (full stack incl. ASL, Arduino on COM8):
```bash
python main.py --webcam --serial COM8
```

**Flags:** `--webcam` (USB/laptop cam) · `--source URL|index` (network/other camera) ·
`--serial PORT` · `--manual` (Xbox controller) · `--objects` (object labels, laptop) ·
`--no-display` (headless).

## Environments (Raspberry Pi 5)

The Pi ships with Python 3.13, but **MediaPipe has no 3.13 wheel**, so ASL needs a
side-by-side Python 3.12:

- `~/philo-venv` — Python 3.13, following only (ultralytics, opencv, pyserial, scikit-learn). No ASL.
- `~/philo312` — Python 3.12 (built from source via `make altinstall`), **full stack incl. MediaPipe**. Use this for everything. Note: `numpy<2` is pinned (MediaPipe requires it).

Install into a venv:
```bash
pip install ultralytics opencv-python mediapipe scikit-learn pyserial pygame "numpy<2"
```

Not committed (see `.gitignore`): model weights (`*.pt`, auto-download), `hand_landmarker.task`
(auto-downloads), `asl_model.pkl` and `asl_features.csv` (regenerate — below).

## Training the ASL model

Built from the [Kaggle ASL Alphabet dataset](https://www.kaggle.com/datasets/grassknoted/asl-alphabet).
Because a model pickled under one numpy version won't load under another (e.g. laptop
Python 3.13 / numpy 2 vs Pi numpy 1.26), training is split so the model is built in the
**same environment that runs it**:

```bash
# 1. Extract features (any machine with the dataset). Also writes asl_features.csv.
python asl_from_dataset.py --dataset "PATH/TO/asl_alphabet_train"

# 2. Train where you'll run it (e.g. copy the CSV to the Pi, then):
~/philo312/bin/python asl_from_dataset.py --csv asl_features.csv
```
`asl_features.csv` is version-independent, so extract once and retrain anywhere.

## Performance (Pi 5)

- **~14 FPS** following only · **~7–11 FPS** with ASL (MediaPipe adds load).
- The object-detection model is **off by default** — running it tanked the Pi to ~0.3 FPS.
- Pose `imgsz=256`. If you ever need more speed, export YOLO to **NCNN** (3–5× on Pi CPU).

## Notes

- **Camera:** USB webcam (plug-and-play, index 0). The `--source` + `webcam_stream.py`
  path can stream a remote camera over a network/direct-Ethernet link.
- **Laptop ↔ Pi:** the home router isolates WiFi clients, so the laptop reaches the Pi over a
  **direct Ethernet cable** (link-local `169.254.x.x`) for SSH/`scp`, or work on the Pi directly.
- The Arduino bench firmware (LEDs + OLED eyes + obstacle override) is in `firmware/arduino/`.
