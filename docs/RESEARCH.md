# Research & References

Sources the team used while designing Philo — mechanical inspiration, academic
background on person-following robots, and component datasheets. Kept here so
the reasoning behind design decisions is traceable, and so anyone reviewing the
project (or asking "did you actually understand this or just copy it?") can see
the source material.

---

## Mechanical Reference Robots

Used by [@hummos430](https://github.com/hummos430) for chassis layout,
differential-drive motor mounting, and general proportions.

| Reference | Why it's relevant |
|---|---|
| [DiffBot — ROS Differential Drive Mobile Robot](https://ros-mobile-robots.com/) | Closest architectural match to Philo: Raspberry Pi + differential drive + wheel encoders + IMU. Used as a reference for motor mount geometry and encoder wiring separated from motor noise. |
| [Modular Differential Drive Robot (Hackaday.io)](https://hackaday.io/project/12533-making-a-modular-differential-drive-robot) | Open CAD files (CC-licensed) for a 4-wheel diff-drive chassis. Referenced for motor pod/bracket isolation design. |
| [Pi Car — 4WD Raspberry Pi Camera Car (Hackster.io)](https://www.hackster.io/bestd25/pi-car-016e66) | Simple 4WD Pi + camera build. Used as a sanity check for wheelbase-to-body-size proportions. |

---

## Person-Following & Gesture Research

Academic background for the vision stack's design choices — largest-bounding-box
tracking (not full re-identification), camera mount height for gesture visibility,
and gesture-based interaction.

| Source | Relevance to Philo |
|---|---|
| [Design of a Vision Based Person Following Robot](https://www.researchgate.net/publication/335892311_Design_of_a_Vision_Based_Person_Following_Robot) | Confirms camera mounting heights in the 700mm–1.5m range are typical for reliable person tracking — used to validate the 380mm mast height/tilt geometry against literature, and to reason about the tradeoffs of a shorter mast on a companion-scale robot. |
| [FollowMe: a Robust Person Following Framework Based on Re-Identification and Gestures](https://arxiv.org/pdf/2311.12992) | Covers gesture-based interaction and person re-identification for following robots. Referenced when scoping Philo's V1 approach ("follow the largest bounding box," no full re-ID) — see [`PRE_BUILD_LOCK.md`](PRE_BUILD_LOCK.md) section D5, which explicitly rejects re-ID as out of scope for this build and cites the difficulty documented in this kind of research. |
| [Person Following Robot with Vision-based and Sensor Fusion Tracking Algorithm](https://cdn.intechopen.com/pdfs/5205/intech-person_following_robot_with_vision_based_and_sensor_fusion_tracking_algorithm.pdf) | Background reading on combining vision with other sensors (Philo's equivalent is the ultrasonic obstacle-override layer running independently of the vision-based following logic). |

---

## Component Datasheets

Datasheets already cited inline in [`hardware/README.md`](../hardware/README.md)
are not duplicated below — this section fills in the ones that weren't linked
there yet.

| Component | Datasheet |
|---|---|
| SSD1306 OLED (128×64, I2C) — used for both companion eyes | [Adafruit SSD1306.pdf](https://cdn-shop.adafruit.com/datasheets/SSD1306.pdf) |
| MPU-6050 IMU — register map (used for the tip-over tilt calculation in `main.cpp`) | [InvenSense MPU-6000/MPU-6050 Register Map](https://www.invensense.com/wp-content/uploads/2015/02/MPU-6000-Register-Map1.pdf) |
| MPU-6050 — full product specification | [MPU-6050 Product Specification (cdiweb mirror)](https://www.cdiweb.com/datasheets/invensense/mpu-6050_datasheet_v3%204.pdf) |

**Already linked in `hardware/README.md`:** ESP32-WROOM-32 (Espressif), TB6612FNG
(Toshiba, via Adafruit), HC-SR04 (Sparkfun), Pololu D24V50F5 (Pololu product page).

---

## How to Use This File

Add a new row when the team pulls in a datasheet, paper, or reference design to
justify a decision — especially if it's the kind of thing someone reviewing the
project (a teacher, judge, or "did AI just do this for you?" skeptic) would
reasonably ask "how did you know to do it that way?"
