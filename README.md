# Meet Philo.
A robot companion that follows you.

> *Philo — from the Greek for friend and loved one.*

---

## What is Philo?

Philo is an autonomous person-following robot. Point a camera at someone and Philo locks on, follows them around a room, and stops the moment they raise their hand. Custom PCB, ESP32 motor control, Raspberry Pi + YOLOv8 vision stack, and a PETG-CF chassis designed in Fusion 360.

**Demo video coming August 2026.**

---

## How it works

```
Camera (RPi) → YOLOv8 pose detection → direction command → ESP32 → motor drivers → wheels
                       ↓
              Raised hand detected → STOP
```

- **Vision:** YOLOv8n-pose running on Raspberry Pi 4. Detects person, calculates horizontal offset from frame center, estimates distance from bounding box height. Detects raised hand via wrist-above-shoulder keypoint check.
- **Control:** ESP32 receives direction + speed over UART. PID loop on each motor side. Ultrasonic sensors override Pi commands if obstacle < 25cm.
- **Hardware:** Custom KiCad PCB with TB6612FNG dual motor drivers, MPU-6050 IMU, 2S LiPo power management.
- **Chassis:** Fusion 360 design, printed in PETG carbon fiber.

---

## Repo structure

```
Philobot/
  hardware/kicad/       ← schematic, PCB files, Gerbers
  firmware/esp32/       ← ESP32 motor control (PlatformIO)
  vision/rpi/           ← YOLOv8 person detection + gesture control
  mechanical/fusion360/ ← chassis CAD files
  mechanical/stl/       ← print-ready STL files
  docs/images/          ← photos, renders, wiring diagrams
```

---

## Team

| Person | Role |
|--------|------|
| [@dish-glitch](https://github.com/dish-glitch) | PCB design + ESP32 firmware |
| [@sivathamizhan](https://github.com/sivathamizhan) | Raspberry Pi vision stack |
| [@hummos430](https://github.com/hummos430) | Chassis design + 3D printing |
| Friend 4 | Mechanical assembly + integration |

---

## Timeline

**Summer 2026 — targeting August 17.**

- Weeks 1-2: PCB schematic, chassis sketch, RPi camera confirmed
- Weeks 3-4: PCB ordered + arrives, chassis printed, motors spinning
- Weeks 5-6: YOLOv8 live, first person following
- Weeks 7-8: Gesture control, obstacle avoidance, polish
- Week 9: Demo video filmed, docs complete

