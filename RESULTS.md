# Philo — Results & Measured Performance

This separates **measured** results (actually observed on hardware) from
**pending** figures that require the assembled PCB + motors. Measured numbers are
reproducible with [`vision/rpi/benchmark.py`](vision/rpi/benchmark.py).

---

## Measured — Raspberry Pi 5 (8GB) + Logitech USB webcam + Arduino Uno

Bench setup: Pi 5 runs the vision/control stack (Python 3.12), USB webcam as the
camera, Arduino Uno standing in for the ESP32 (drives LEDs + OLED + ultrasonic
obstacle override). Measured 2026-06.

| Metric | Measured | Method |
|---|---|---|
| Person-following pipeline (YOLOv8n-pose, `imgsz=256`, pose-only) | **~14 FPS** sustained | `main.py` FPS counter, steady over several minutes |
| Full stack + ASL (YOLOv8n-pose **+** MediaPipe hands per frame) | **~7–11 FPS** | `main.py` FPS counter |
| Object-labeling model added (YOLOv8s @ `imgsz=640`) | **~0.3 FPS** | inferred from ~15 s gesture lag; why the object model is **off by default** on the Pi |
| Gesture-stop response latency | **~0.35 s** | 5-frame confirmation ÷ 14 FPS |
| ASL recognition confidence (distinct letters B, C, U, N) | **0.94 – 1.00** | live softmax probability on the Pi |
| ASL training set | **11,742 samples** (13,000 dataset images; 1,258 with no detectable hand skipped) | `asl_from_dataset.py` |
| ASL training convergence | loss **1.63 → 0.0016** in 74 iterations | MLPClassifier, early-stop on plateau |
| Pi → Arduino link | **115200 baud**, `STATUS` telemetry at **10 Hz** | protocol, verified live |
| Obstacle override threshold | **20 cm** (LED + OLED "STOP" fire within threshold, independent of camera) | firmware + live bench test |

**Key engineering result:** dropping the second (object-detection) model and
lowering `imgsz` 320→256 took the Pi from **~0.3 FPS to ~14 FPS** (~45×) — the
difference between an unusable ~15 s gesture lag and a ~0.35 s response.

## Verified in design (pre-bring-up, provable now)

| Item | Result | Evidence |
|---|---|---|
| Schematic / PCB | ERC clean, DRC clean, Gerbers exported | KiCad build log, June 21 |
| Motor driver choice | TB6612FNG over L298N | eliminates ~2 V bridge drop, no heatsink — real tradeoff |
| Camera geometry | 380 mm mount height, 25° upward tilt | trig-derived to frame knee→above-head at 1.5 m |
| Safety thresholds | 500 ms watchdog, 20–30 cm obstacle stop, 30° tilt cutoff | firmware, compiles clean |

## Pending hardware bring-up (targets — not yet measured)

These need the assembled board + motors and will be filled in with real numbers
after bring-up:

- **PID gains** (starting point Kp=1.0, Ki=0.1, Kd=0.05) — tune on the bench
- **Motor stall / running current** per channel
- **Camera → wheel-response latency** (end-to-end, stopwatch/logged)
- **Battery runtime** and full-load power draw

## Reproduce the measured numbers

On the Pi (3.12 env), no Arduino needed:
```bash
~/philo312/bin/python vision/rpi/benchmark.py --webcam --frames 300        # following FPS + per-stage ms
~/philo312/bin/python vision/rpi/benchmark.py --webcam --frames 300 --asl  # add MediaPipe timing
```
The script reports sustained FPS and mean/median/min/max latency for each stage
(camera grab, YOLO-pose, MediaPipe, total loop).
