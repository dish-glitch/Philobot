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

The follow-a-person problem is deceptively hard. The camera has to detect a human, figure out where they are relative to the robot, translate that into motor commands, keep the robot from walking into walls, and do all of it fast enough that the robot does not lag behind. Every layer of that stack — PCB, firmware, vision, chassis — had to be designed and integrated by us. No shields. No pre-built frames. No off-the-shelf motor hat.

The gesture control was the detail that made the project feel finished. A robot that follows you is a demo. A robot that freezes when you raise your hand and resumes when you lower it feels like a product.

---

## What It Does

A Raspberry Pi 4 runs YOLOv8 pose estimation on a live camera feed, detects the nearest person in frame, and continuously streams direction and speed commands to an ESP32 over UART. The ESP32 runs closed-loop PID speed control on four DC motors via dual H-bridge drivers. Three ultrasonic sensors on the front of the chassis act as an independent safety layer — if an obstacle is detected under 25cm, the ESP32 overrides the Pi command and stops regardless of what the vision stack says.

**Gesture control:** YOLOv8 tracks 17 body keypoints per person. When either wrist keypoint rises above the shoulder keypoint in frame coordinates, the Pi switches to STOP mode and holds it for 5 consecutive frames before acting — this debounces the detection and prevents flickering. Lowering the hand for 5 consecutive frames resumes following.

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
                |  Format: CMD:<F/B/L/R/S>,<0-255>
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
| N20 Gear Motors w/ Encoders (12V 200RPM) | Drive wheels | 4 | ~$4 ea | AliExpress |
| 65mm Rubber Wheels | Traction | 4 | ~$2 ea | AliExpress |
| HC-SR04 Ultrasonic Sensors | Obstacle detection | 3 | ~$1 ea | Amazon |
| 2S LiPo 2200mAh | Main battery | 1 | ~$14 | Amazon / HobbyKing |
| 2S LiPo Balance Charger | Charging | 1 | ~$10 | Amazon |
| Raspberry Pi 4 (2GB+) | Vision compute | 1 | ~$45 | [Adafruit](https://www.adafruit.com) |
| Raspberry Pi Camera Module v2 | 8MP, 120 degree FOV | 1 | ~$25 | Amazon |
| AMS1117-3.3 LDO | 3.3V rail for ESP32 logic | 1 | ~$1 | DigiKey |
| MP2307 Buck Converter | 5V rail for Raspberry Pi | 1 | ~$2 | DigiKey |
| SMD Resistors 0603 assortment | Pull-ups, current limiting | — | ~$9 | Amazon |
| SMD Capacitors 0603/0805 assortment | Decoupling | — | ~$10 | Amazon |
| XT30 Connector Pair | Battery connector | 1 | ~$2 | Amazon |
| 2.54mm Pin Headers | Sensor and UART connectors | — | ~$6 | Amazon |
| CP2102 USB-UART Programmer | Flash ESP32 firmware | 1 | ~$7 | Amazon |
| Custom PCB — JLCPCB (5 boards) | From Gerbers | 1 run | ~$25 | [JLCPCB](https://jlcpcb.com) |
| PETG-CF Filament | Chassis | — | $0 | On hand |
| M2.5 / M3 Standoffs + Screws | Mounting hardware | — | ~$8 | Amazon |
| **Total** | | | **~$150** | |

---

## Repository Structure

```
Philobot/
├── hardware/
│   └── kicad/          # Schematic, PCB layout, Gerber files
├── firmware/
│   └── esp32/          # PlatformIO project — motor control, PID, serial, ultrasonic
├── vision/
│   └── rpi/            # Python — YOLOv8 inference, gesture detection, serial output
├── mechanical/
│   ├── fusion360/      # Chassis CAD source files
│   └── stl/            # Print-ready STL files
└── docs/
    └── images/         # Photos, renders, wiring diagrams
```

---

## Build Log

| Date | Milestone |
|---|---|
| June 16, 2026 | Project started. Repository initialized. Team of 4 assembled. |
| June 17, 2026 | KiCad schematic started — ESP32, motor drivers, power block |
| — | Chassis dimensions locked in Fusion 360 |
| — | RPi OS flashed, camera confirmed with libcamera-hello |
| — | PCB schematic complete, ERC clean |
| — | PCB layout complete, DRC clean, Gerbers exported |
| — | PCB ordered from JLCPCB |
| — | Chassis v1 printed in PETG-CF |
| — | PCB arrived, soldered, power-on test passed |
| — | Basic motor spin confirmed over serial |
| — | YOLOv8n-pose running live on RPi, 10+ FPS confirmed |
| — | First person following — open loop |
| — | Gesture control working — hand raise stops robot |
| — | Obstacle avoidance tuned |
| — | Demo video filmed |
| Aug 17, 2026 | School starts — project complete |

---

## Status

- [ ] KiCad schematic
- [ ] PCB layout
- [ ] PCB fabrication (JLCPCB)
- [ ] Chassis design (Fusion 360)
- [ ] Chassis printed
- [ ] Motor control firmware (ESP32)
- [ ] PID speed loop
- [ ] Ultrasonic obstacle avoidance
- [ ] YOLOv8 person detection live on RPi
- [ ] Gesture control (hand raise stops, lower resumes)
- [ ] Full system integration
- [ ] Demo video

---

## Team

| Person | Role |
|---|---|
| [@dish-glitch](https://github.com/dish-glitch) | PCB design (KiCad) + ESP32 firmware |
| [@sivathamizhan](https://github.com/sivathamizhan) | Raspberry Pi vision stack — YOLOv8, gesture detection, serial comms |
| [@hummos430](https://github.com/hummos430) | Chassis design in Fusion 360 + 3D printing in PETG-CF |
| Friend 4 | Mechanical assembly + integration testing |

---

## License

MIT — use, modify, share freely. See [LICENSE](LICENSE).
