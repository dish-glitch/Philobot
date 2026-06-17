# Vision — Philo Raspberry Pi Stack

**Owner: [@sivathamizhan](https://github.com/sivathamizhan)**

---

## Overview

The Raspberry Pi 4 is Philo's eyes and brain. It runs a camera feed through YOLOv8 pose estimation, figures out where the person is relative to the robot's center, and sends short commands over serial to the ESP32 telling it which way to go.

It also handles gesture control entirely in software — no extra hardware needed. When the person raises their hand, the Pi detects it and sends a stop command. When they lower it, normal following resumes.

---

## Camera Installation Requirements

**This must be done before any software development. If the camera is in the wrong physical position, gesture detection will not work regardless of how the code is written.**

### Why camera height matters

At a camera height of 200mm (robot body), the camera mounted level sees a 1.7m person from ~40mm to ~720mm height at 1.5m distance — only legs. Raising a hand is invisible.

At 380mm height with 25 degree upward tilt:
```
Camera at 380mm, tilted 25 degrees up, person 1.5m away:
  Bottom of frame: 380 + 1500 x tan(0.6 degrees) = ~396mm (knee height)
  Center of frame: 380 + 1500 x tan(25 degrees) = ~1079mm (mid-torso)
  Top of frame:    380 + 1500 x tan(49.4 degrees) = ~2130mm (above head)
```

The camera sees from knee height to above the head. Wrists and shoulders are clearly visible. Gesture detection works.

### Camera mounting specification (give this to hummos430)

- Camera height from ground: **380mm** (requires a ~200mm mast above the chassis body)
- Camera tilt: **20-25 degrees upward** from horizontal
- Mast minimum cross-section: 20mm x 20mm PETG-CF (to prevent flex during movement)
- CSI ribbon cable must be strain-relieved within 20mm of both connectors

### Effect on steering logic

The tilt is vertical only. The horizontal left-right position mapping is unchanged — the offset calculation still works exactly as documented below. Vertical tilt does not affect the horizontal plane.

---

## Setup (Do This First)

1. Flash Raspberry Pi OS Lite 64-bit to SD card (use Raspberry Pi Imager)
2. Enable camera interface: `sudo raspi-config` -> Interface Options -> Camera
3. **Disable serial console** (required — Pi sends boot messages over UART which corrupt ESP32 commands):
   `sudo raspi-config` -> Interface Options -> Serial Port -> Shell: No, Hardware: Yes
4. Confirm camera works: `libcamera-hello --timeout 5000`
5. Install dependencies:

```bash
pip install ultralytics opencv-python pyserial picamera2
```

6. Download the model (first run downloads automatically, or manually):

```bash
python -c "from ultralytics import YOLO; YOLO('yolov8n-pose.pt')"
```

---

## Camera Configuration (Exposure Lock)

**Run this at startup before the inference loop. Do not skip it.**

When the robot moves from a bright window to a dim hallway, auto-exposure takes 10-20 frames to adjust. During those frames, YOLO receives washed-out or underexposed images and detection fails — the robot stops following and loses the person.

Fix: let the camera auto-expose for 30 frames on startup, then lock the settings for the session.

```python
from picamera2 import Picamera2

picam2 = Picamera2()
config = picam2.create_video_configuration(main={"size": (640, 480)})
picam2.configure(config)
picam2.start()

# Let auto-exposure stabilize
for _ in range(30):
    frame = picam2.capture_array()

# Lock exposure settings
picam2.set_controls({
    "AeEnable": False,          # disable auto-exposure
    "AwbEnable": False,         # disable auto white balance
    "ExposureTime": picam2.capture_metadata()["ExposureTime"],
    "AnalogueGain": picam2.capture_metadata()["AnalogueGain"],
    "ColourGains": picam2.capture_metadata()["ColourGains"],
})
```

**Trade-off:** locked exposure means the image will get brighter or darker if room lighting changes. For a living room demo under consistent indoor lighting this is fine. If the robot will operate in dramatically varying light (e.g., outside), do not lock exposure — accept the 10-20 frame detection gap instead.

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
2. Select the target person (see Person Selection below)
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

## Person Selection — Handling Multiple People in Frame

YOLOv8 detects every person visible. If a family member walks through the room, Philo might switch from following you to following them.

**V1 approach: follow the largest bounding box (closest person)**

The person who gave the robot its last gesture command is almost certainly the closest person in the frame. Follow whichever detection has the largest bounding box area.

```python
results = model(frame)
if results[0].boxes:
    boxes = results[0].boxes
    areas = boxes.xywh[:, 2] * boxes.xywh[:, 3]  # width x height
    target_idx = areas.argmax().item()
    # use boxes[target_idx] and keypoints[target_idx]
```

**Known limitation:** This is not true re-identification. If two people are at similar distances and one passes in front of the other, the robot may switch targets briefly. This is a known unsolved problem in person-following robotics. For a demo, it is acceptable. If needed, a future improvement would be to lock onto the person's approximate shirt color as a secondary identifier.

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

**Why gesture detection requires the elevated camera:** Gesture is detected by comparing wrist Y-coordinate to shoulder Y-coordinate in the image. If the camera only sees legs (low mount, no tilt), neither wrist nor shoulder keypoints are in frame and all confidence scores are 0. The gesture check will never trigger. The camera mast is not optional.

---

## What Happens When No Person Is Detected

If no person is in frame:

```python
if no detections:
    send_serial("CMD:S,0")   # stop and wait
```

Do not keep sending the last movement command — that causes the robot to drive blind. Stop and wait for the person to re-enter frame.

**Optional improvement (week 7):** if person is lost, spin slowly in the last known direction for up to 2 seconds while searching. If not found, stop.

**Known limitation — occlusion:** If the person walks behind a wall or large piece of furniture and another person is standing nearby, Philo may start following the second person when the first reappears. There is no reliable fix for this within the V1 scope. The robot's behavior when it loses the target is to stop, which is safe. Accept this limitation and document it in the demo.

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
+-- requirements.txt         # ultralytics, opencv-python, pyserial, picamera2
+-- follow.py                # main script -- camera, inference, serial output
+-- gesture.py               # hand raise detection, state machine
+-- utils.py                 # offset calculation, distance mapping, serial helpers
```

---

## Performance Targets

| Metric | Target |
|---|---|
| Inference FPS | 10+ FPS |
| Latency per frame | < 100ms |
| Minimum confidence to track | 0.5 |
| Dead zone | 0.15 (+/-15% of frame center) |
| Max comfortable follow distance | ~3 meters |

If FPS drops below 8, reduce input size: `model(frame, imgsz=224)` instead of 320.

---

## Timeline

| Week | Target |
|---|---|
| 1 | RPi OS flashed, camera confirmed with libcamera-hello, serial console disabled |
| 2-3 | YOLOv8n-pose running, person detected on test image |
| 4 | Camera mast installed, exposure lock confirmed working |
| 4-5 | Live inference from camera, offset values printing to terminal |
| 5-6 | Serial output to ESP32 working, robot responds to person position |
| 6 | Gesture control — raised hand stops robot |
| 7 | Tuning dead zone, distance thresholds, lost-person behavior |
| 8 | Stable enough for demo video |
