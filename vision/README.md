# Vision — Philo Raspberry Pi Stack

**Owner: [@sivathamizhan](https://github.com/sivathamizhan)**

---

## Overview

The Raspberry Pi 4 is Philo's eyes and brain. It runs a camera feed through YOLOv8 pose estimation, figures out where the person is relative to the robot's center, and sends short commands over serial to the ESP32 telling it which way to go.

It also handles gesture control entirely in software — no extra hardware needed. When the person raises their hand, the Pi detects it and sends a stop command. When they lower it, normal following resumes.

---

## Setup (Do This First)

1. Flash Raspberry Pi OS Lite 64-bit to SD card (use Raspberry Pi Imager)
2. Enable camera interface: `sudo raspi-config` → Interface Options → Camera
3. Confirm camera works: `libcamera-hello --timeout 5000`
4. Install dependencies:

```bash
pip install ultralytics opencv-python pyserial
```

5. Download the model (first run downloads automatically, or manually):

```bash
python -c "from ultralytics import YOLO; YOLO('yolov8n-pose.pt')"
```

---

## Model Choice

**yolov8n-pose.pt** — YOLOv8 nano pose estimation model.

- Detects people AND their 17 body keypoints in a single pass
- Runs at 10-15 FPS on Raspberry Pi 4 at imgsz=320
- This is fast enough for following a walking person
- Do not use the larger models (s, m, l) — they are too slow on Pi hardware without a GPU

The 17 keypoints follow the COCO standard. The ones Philo uses:

| Index | Keypoint |
|---|---|
| 5 | Left shoulder |
| 6 | Right shoulder |
| 9 | Left wrist |
| 10 | Right wrist |

---

## Person Following Logic

Every frame:

1. Run YOLOv8n-pose inference on the camera frame
2. Find the person detection with the highest confidence score
3. Get their bounding box center X position
4. Calculate offset from frame center:

```python
offset = (box_center_x - frame_width / 2) / (frame_width / 2)
# offset range: -1.0 (person far left) to +1.0 (person far right)
# offset near 0.0 = person is centered = go straight
```

5. Estimate distance using bounding box height:

```python
box_height_ratio = box_height / frame_height
# ratio > 0.7 = person very close = slow down or stop
# ratio < 0.2 = person far away = speed up
# ratio 0.3-0.5 = comfortable following distance = normal speed
```

6. Translate offset and distance into a command:

```python
DEAD_ZONE = 0.15  # ignore small offsets, prevents jitter

if box_height_ratio > 0.7:
    command = "CMD:S,0"          # too close, stop
elif offset > DEAD_ZONE:
    command = "CMD:R,150"        # person right, turn right
elif offset < -DEAD_ZONE:
    command = "CMD:L,150"        # person left, turn left
else:
    speed = map_distance_to_speed(box_height_ratio)
    command = f"CMD:F,{speed}"   # centered, go forward
```

7. Send command over serial to ESP32

---

## Gesture Control Logic

**Condition for raised hand:**
In image coordinates, Y=0 is the TOP of the frame and increases downward. So a wrist that is physically above the shoulder will have a LOWER Y value in the image.

```python
def hand_raised(keypoints):
    left_wrist_y    = keypoints[9][1]
    left_shoulder_y = keypoints[5][1]
    right_wrist_y   = keypoints[10][1]
    right_shoulder_y = keypoints[6][1]

    # Only trust keypoints with confidence > 0.5
    left_wrist_conf  = keypoints[9][2]
    right_wrist_conf = keypoints[10][2]

    left_raised  = (left_wrist_y < left_shoulder_y) and (left_wrist_conf > 0.5)
    right_raised = (right_wrist_y < right_shoulder_y) and (right_wrist_conf > 0.5)

    return left_raised or right_raised
```

**State machine — prevents flickering:**

Do not switch state on a single frame. Require 5 consecutive frames of the same gesture before changing state.

```python
state = "FOLLOWING"
raised_frames  = 0
lowered_frames = 0
THRESHOLD = 5

# Inside the loop:
if hand_raised(keypoints):
    raised_frames  += 1
    lowered_frames  = 0
else:
    lowered_frames += 1
    raised_frames   = 0

if raised_frames  >= THRESHOLD:
    state = "WAITING"
if lowered_frames >= THRESHOLD:
    state = "FOLLOWING"

if state == "WAITING":
    send_serial("CMD:S,0")
else:
    send_serial(follow_command)
```

---

## What Happens When No Person Is Detected

If no person is in frame:

```python
if no detections:
    send_serial("CMD:S,0")   # stop and wait
```

Do not keep sending the last movement command — that causes the robot to drive blind. Stop and wait for the person to re-enter frame.

Optional improvement (week 7): if person is lost, spin slowly in the last known direction for up to 2 seconds while searching. If not found, stop.

---

## Serial Output to ESP32

```python
import serial

ser = serial.Serial('/dev/ttyS0', 115200, timeout=1)
# On some Pi setups the port is /dev/ttyAMA0 — check with: ls /dev/tty*

def send_serial(command):
    ser.write((command + '\n').encode('utf-8'))
```

Send one command per inference loop iteration. Do not flood the serial port — one command per frame is enough.

---

## File Structure

```
vision/rpi/
├── requirements.txt         # ultralytics, opencv-python, pyserial
├── follow.py                # main script — camera, inference, serial output
├── gesture.py               # hand raise detection, state machine
└── utils.py                 # offset calculation, distance mapping, serial helpers
```

---

## Performance Targets

| Metric | Target |
|---|---|
| Inference FPS | 10+ FPS |
| Latency per frame | < 100ms |
| Minimum confidence to track | 0.5 |
| Dead zone | 0.15 (±15% of frame center) |
| Max comfortable follow distance | ~3 meters |

If FPS drops below 8, reduce input size: `model(frame, imgsz=224)` instead of 320.

---

## Timeline

| Week | Target |
|---|---|
| 1 | RPi OS flashed, camera confirmed with libcamera-hello |
| 2-3 | YOLOv8n-pose running, person detected on test image |
| 4-5 | Live inference from camera, offset values printing to terminal |
| 5-6 | Serial output to ESP32 working, robot responds to person position |
| 6 | Gesture control — raised hand stops robot |
| 7 | Tuning dead zone, distance thresholds, lost-person behavior |
| 8 | Stable enough for demo video |
