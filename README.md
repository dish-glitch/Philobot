# Philo — Autonomous Person-Following Robot

**A 4-wheeled ground robot that locks onto a person using real-time YOLOv8 pose detection and follows them across a room. Raise your hand and it stops. Lower it and it follows again.**

![Status](https://img.shields.io/badge/status-in%20progress-yellow)
![License](https://img.shields.io/badge/license-MIT-blue)
![KiCad](https://img.shields.io/badge/PCB-KiCad-blue)
![Platform](https://img.shields.io/badge/firmware-ESP32-green)
![Vision](https://img.shields.io/badge/vision-YOLOv8-purple)
![CAD](https://img.shields.io/badge/CAD-Fusion%20360-orange)

---

## Schematic Preview

<img width="1435" height="1063" alt="image" src="https://github.com/user-attachments/assets/c237f2eb-55cb-4f29-82f1-52ff88f5388f" />

---

## PCB Preview

<img width="730" height="597" alt="image" src="https://github.com/user-attachments/assets/4cc0b4a3-e955-4f66-854f-2159c8c0b7e5" />


---

## Chassis Preview

*Coming soon — Fusion 360 render in progress*

---

## Why We Built This

Most robotics projects you see online are either a pre-built kit with a tutorial, or a graduate research platform that costs $10,000 and runs ROS on a laptop strapped to a frame. We wanted the middle ground — a robot that does something real and visually impressive, built from scratch at the hardware level, for under $150.

The follow-a-person problem is deceptively hard. The camera has to detect a human, figure out where they are relative to the robot, translate that into motor commands, keep the robot from walking into walls, and do all of it fast enough that the robot does not lag behind. 



---

## What It Does

A Raspberry Pi 4 runs YOLOv8 pose estimation on a live camera feed, detects the nearest person in frame, and continuously streams direction and speed commands to an ESP32 over UART. The ESP32 runs closed-loop PID speed control on four DC motors via dual H-bridge drivers. Three ultrasonic sensors on the front of the chassis act as an independent safety layer — if an obstacle is detected under 25cm, the ESP32 overrides the Pi command and stops regardless of what the vision stack says.

**Gesture control:** YOLOv8 tracks 17 body keypoints per person. When either wrist keypoint rises above the shoulder keypoint in frame coordinates, the Pi switches to STOP mode and holds it for 5 consecutive frames before acting — this debounces the detection and prevents flickering. Lowering the hand for 5 consecutive frames resumes following. Stop gesture confirmed working in testing.

**Sign language recognition:** The camera also detects ASL hand signs in real time using MediaPipe hand landmarks and a trained classifier. Recognized letters are sent over UART to the ESP32 and displayed live on the onboard OLED display. Currently works for a subset of letters — confidence sits around 50% for some signs and model training is ongoing to expand coverage and improve reliability. *Note: this feature is experimental. We know it is limited and are actively working to improve it.*

**Obstacle avoidance:** Three HC-SR04 ultrasonic sensors (front-left at -30 degrees, front-center, front-right at +30 degrees) feed into a priority layer in ESP32 firmware. The Pi handles direction. The ESP32 handles not hitting things. These are separate concerns and intentionally kept that way.

---

## How It Works

```
+--------------------------------------------------+
|              Raspberry Pi 4                      |
|                                                  |
|  Camera -> YOLOv8n-pose -> keypoint extraction   |
|               |                                  |
|      Person bounding box offset                  |
|      Wrist vs. shoulder Y position               |
|               |                                  |
|      State machine (FOLLOWING / WAITING)         |
|               |                                  |
|      Serial command -> UART TX                   |
+---------------+----------------------------------+
                |  115200 baud UART
                |  Format: CMD <left> <right> <flags>  (-255..255)
                v
+--------------------------------------------------+
|              ESP32-WROOM-32                      |
|                                                  |
|  UART RX -> parse command                        |
|               |                                  |
|  Ultrasonic check (priority override)            |
|               |                                  |
|  PID speed control per motor side                |
|  PWM via LEDC peripheral at 20kHz               |
|               |                                  |
|  TB6612FNG x2 -> 4 DC motors with encoders      |
+--------------------------------------------------+
```

**Latency budget (targets):**

| Stage | Target |
|---|---|
| Camera capture | ~5ms |
| YOLOv8n-pose inference (imgsz=320) | ~70ms |
| Serial transmission | ~1ms |
| ESP32 motor response | ~5ms |
| **Total loop** | **~80ms (~12 Hz)** |

12 Hz is sufficient for following a walking person. A running person would expose the lag — but that is not the use case.

**Why UART and not I2C or SPI:** UART is asynchronous and trivially debuggable with a serial monitor. I2C requires the ESP32 to act as master which complicates the Pi-side code. SPI is faster than needed. UART at 115200 baud moves our 20-byte command packets in under 2ms. It is the right tool for this interface.

**Why TB6612FNG and not L298N:** The L298N drops ~2V across its internal transistors at 1A, wasting power as heat and requiring a heatsink. The TB6612FNG is a MOSFET H-bridge — nearly zero voltage drop, no heatsink needed, physically smaller. It is the correct motor driver for battery-powered mobile robots.

**Why YOLOv8n-pose and not separate detection and pose models:** One model pass gives you person detection and 17 keypoints simultaneously. Running two models would double inference time and add synchronization complexity. The nano pose model is marginally slower than nano detect-only and eliminates the entire second pipeline.

---

## Hardware

> Prices are estimates. Amazon prices fluctuate — verify before ordering. Order DigiKey parts together to share one flat shipping fee (~$8).

| Component | Purpose | Qty | Est. Price | Where |
|---|---|---|---|---|
| ESP32-WROOM-32 | Main MCU | 1 | ~$5 | Amazon |
| TB6612FNG Dual H-Bridge | Motor driver (2 motors each) | 2 | ~$2 ea | [Adafruit](https://www.adafruit.com/product/2448) |
| MPU-6050 IMU | Tilt detection, auto-stop on tip-over | 1 | ~$2 | Amazon |
| JGA25-370 Gear Motors w/ Encoders (6V 200RPM) | Drive wheels | 4 | ~$8 ea | AliExpress |
| 80mm Rubber Wheels | Traction | 4 | ~$3 ea | AliExpress |
| HC-SR04 Ultrasonic Sensors | Obstacle detection | 3 | ~$1 ea | Amazon |
| 2S LiPo 2200mAh | Main battery | 1 | ~$14 | Amazon / HobbyKing |
| 2S LiPo Balance Charger | Charging | 1 | ~$10 | Amazon |
| Raspberry Pi 5 (4GB) | Vision compute | 1 | on hand | From friend — with active cooler |
| Raspberry Pi Camera Module 3 | 12MP, autofocus | 1 | on hand | Attached, confirmed working |
| AMS1117-3.3 LDO | 3.3V rail for ESP32 logic | 1 | ~$1 | DigiKey |
| Pololu D24V50F5 Buck (5V 5A) | 5V rail for Raspberry Pi | 1 | ~$15 | Pololu |
| SMD Resistors 0603 assortment | Pull-ups, current limiting | — | ~$9 | Amazon |
| SMD Capacitors 0603/0805 assortment | Decoupling | — | ~$10 | Amazon |
| XT30 Connector Pair | Battery connector | 1 | ~$2 | Amazon |
| 2.54mm Pin Headers | Sensor and UART connectors | — | ~$6 | Amazon |
| CP2102 USB-UART Programmer | Flash ESP32 firmware | 1 | ~$7 | Amazon |
| Custom PCB — JLCPCB (5 boards) | From Gerbers | 1 run | ~$25 | [JLCPCB](https://jlcpcb.com) |
| PETG-CF Filament | Chassis | — | $0 | On hand |
| M2.5 / M3 Standoffs + Screws | Mounting hardware | — | ~$8 | Amazon |
| **Total** | | | **~$180** | |

---

## Repository Structure

```
Philo/
├── hardware/
│   └── kicad/
│       ├── Philo.kicad_sch           # Schematic (ESP32, motors, power, sensors)
│       ├── Philo- main.kicad_pcb     # PCB layout — 100×100mm 2-layer
│       └── gerbers/                  # Production files ready for JLCPCB
│           ├── *.gtl / *.gbl         # Front / back copper
│           ├── *.gts / *.gbs         # Solder mask
│           ├── *.gto / *.gbo         # Silkscreen
│           ├── *.gm1                 # Board outline
│           └── *.drl                 # Drill files
├── firmware/
│   └── esp32/
│       ├── platformio.ini            # PlatformIO config (espressif32 6.9.0)
│       └── src/
│           ├── main.cpp              # Entry point — 10 Hz sensor + Pi loop
│           ├── pins.h                # All GPIO assignments (verified from netlist)
│           ├── motors.h / .cpp       # TB6612FNG H-bridge control (PWM via LEDC)
│           ├── encoders.h / .cpp     # Interrupt-based wheel encoder counting
│           ├── ultrasonic.h / .cpp   # HC-SR04 distance (L / C / R)
│           ├── imu.h / .cpp          # MPU-6050 over I²C (accel + gyro)
│           ├── display.h / .cpp      # SSD1306 OLED status display
│           └── pi_comm.h / .cpp      # UART text protocol to Raspberry Pi
├── mechanical/
│   ├── Philo- main.step              # 3D board export
│   ├── fusion360/                    # Chassis CAD source files
│   └── stl/                         # Print-ready STL files
├── vision/
│   └── rpi/                         # YOLOv8 inference + gesture detection (Python)
└── docs/
    ├── images/                       # Photos, renders, wiring diagrams
    └── PRE_BUILD_LOCK.md             # Design freeze checklist
```

---

## Build Log

| Date | Milestone |
|---|---|
| June 16, 2026 | Project started. Repository initialized. Team of 4 assembled. |
| June 17, 2026 | KiCad schematic started — ESP32, motor drivers, power block |
| June 21, 2026 | PCB schematic complete, ERC clean |
| June 21, 2026 | PCB layout complete, DRC clean, Gerbers exported |
| June 22, 2026 | ESP32 firmware skeleton complete — all modules compile (motors, encoders, ultrasonic, IMU, OLED, Pi UART) |
| June 22, 2026 | Gesture stop function confirmed working (tested on Arduino) |
| June 22, 2026 | Sign language recognition working for subset of letters — OLED display output confirmed. Training ongoing for full alphabet. |
| June 22, 2026 | Chassis design started in Fusion 360 |
| — | PCB ordered from JLCPCB |
| — | Chassis v1 printed in PETG-CF |
| — | PCB arrived, soldered, power-on test passed |
| — | Basic motor spin confirmed over serial |
| — | YOLOv8n-pose running live on RPi, 10+ FPS confirmed |
| — | First person following — open loop |
| — | Gesture control working on robot |
| — | Sign language full alphabet trained and deployed |
| — | Obstacle avoidance tuned |
| — | Demo video filmed |
| Aug 17, 2026 | School starts — project complete |

---

## Status

- [x] KiCad schematic — complete, ERC clean
- [x] PCB layout — complete, DRC clean, Gerbers exported
- [x] ESP32 firmware skeleton — all modules compile (motors, encoders, ultrasonic, IMU, OLED, Pi UART)
- [x] Pi vision stack — YOLOv8 person following + gesture + ASL running on laptop/Arduino
- [x] Gesture stop — confirmed working in bench testing (Arduino stand-in)
- [x] ASL sign recognition — working for subset of letters; training ongoing (see issues #1 #3 #4)
- [ ] Gesture sensitivity tuning — currently too sensitive, fix tracked in issue #2
- [ ] PCB fabrication (JLCPCB)
- [ ] Chassis design (Fusion 360) — in progress
- [ ] Chassis printed
- [ ] PID speed loop (ESP32)
- [ ] Ultrasonic obstacle avoidance tuning
- [ ] Full system integration on robot hardware
- [ ] Demo video

---

## Team

| Person | Role |
|---|---|
| [@dish-glitch](https://github.com/dish-glitch) | PCB design (KiCad) + ESP32 firmware |
| [@sivathamizhan](https://github.com/sivathamizhan) & [@dish-glitch](https://github.com/dish-glitch) | Raspberry Pi vision stack — YOLOv8, gesture detection, serial comms |
| [@hummos430](https://github.com/hummos430) | Chassis design in Fusion 360 + 3D printing in PETG-CF |
| [@dish-glitch](https://github.com/dish-glitch), [@hummos430](https://github.com/hummos430), [@sivathamizhan](https://github.com/sivathamizhan) | Mechanical assembly + integration testing |

---

## AI Tools Used

We used Claude (Anthropic) as a reference tool during this project and are disclosing it openly. We had no robotics program, no teacher, and no mentor. AI filled the role a textbook or more experienced teammate would have filled if we'd had access to one.

**Where Claude helped:**

- Reviewing GPIO assignments from the KiCad netlist and catching wiring errors before we soldered anything
- Drafting initial ESP32 firmware modules as a starting point — the team then read through, modified, tested, and debugged everything on actual hardware
- Diagnosing a PlatformIO build failure (`pins_arduino.h` not found) caused by a version conflict between espressif32@6.9.0 and 7.0.1
- Explaining why specific hardware choices make sense (TB6612FNG vs L298N, YOLOv8n-pose vs separate models, UART vs I2C) so we could make informed decisions
- Helping structure the GitHub repository and draft documentation to a presentable structure. 
- Providing weight and dimension constraints for the chassis — specifically, what size the enclosure needs to be so the motors can actually move it. The chassis itself is fully designed by the team in Fusion 360

**What the team built and owns:**

- All hardware decisions — component selection, schematic design, and PCB layout in KiCad
- The full vision stack — YOLOv8 inference pipeline, ASL classifier, gesture detection, serial comms — written and tested by the team
- All physical testing and iteration — every debug session, hardware fix, and tuning pass was done by us in the real world
- The chassis design in Fusion 360 — built by our mechanical lead from scratch
- Every journal entry — written by the team in our own words

We are happy to answer questions about any part of this project. We have used AI to help build it, but we understand everything we built. If AI helped create any line of code or suggested a part that seemed better, we did the research to understand what that part or code does and why — before it went into the project.

---

## License

MIT — use, modify, share freely. See [LICENSE](LICENSE).
