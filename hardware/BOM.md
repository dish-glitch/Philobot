# Philo — Bill of Materials (Complete)

**This is the authoritative parts list for the PCB.** Every component that goes on or connects to the custom board is here, including passives. Use this when placing symbols in KiCad and when ordering parts.

Legend — **Form:** IC = bare chip soldered to board; Module = pre-made breakout placed on board; Header = connector for an off-board part.

---

## Active Components

| Ref | Part | Value / Spec | Form | Package | Qty | Notes |
|---|---|---|---|---|---|---|
| U1 | ESP32-WROOM-32 | 240MHz, 4MB flash | Module | Castellated SMD | 1 | Use Espressif KiCad footprint |
| U2 | MP1584EN | 5.0V out, 3A buck | Module (recommended) | Pin/castellated module | 1 | Module includes inductor, diode, FB resistors. Set output to 5.0V with onboard trimpot BEFORE soldering down. If using bare IC instead, add L/D/R_fb per datasheet. |
| U3 | AMS1117-3.3 | 3.3V LDO, 800mA | IC | SOT-223 | 1 | **Input from 5V rail (U2 out), NOT VBAT** — thermal lock decision |
| U4 | TB6612FNG | Dual H-bridge | IC | SSOP-24 (exposed pad) | 1 | Channel A = left-front motor, Channel B = right-front motor |
| U5 | TB6612FNG | Dual H-bridge | IC | SSOP-24 (exposed pad) | 1 | Channel A = left-rear motor, Channel B = right-rear motor |
| U6 | MPU-6050 (GY-521) | 6-axis IMU | Module | Breakout | 1 | I2C 0x68 (AD0 to GND). Mount flat/horizontal |
| Q1 | P-channel MOSFET | Vds>30V, >10A, low Rds(on) | IC | TO-252 / SO-8 | 1 | Reverse-polarity protection on VBAT. Candidate: AOD4185. Carries full system current — do not under-spec |
| D1 | Status LED | any color | LED | 0805 / 3mm | 1 | On GPIO2 via R10 |

---

## Off-Board Modules (connect via headers, not on PCB)

| Part | Spec | Qty | Connects via | Notes |
|---|---|---|---|---|
| OLED eye | SSD1306/SH1106, 128x64, I2C | 2 | J_MAST (I2C) | One at 0x3C, one at 0x3D (set address pad on each). Mounted on camera mast |
| HC-SR04 | Ultrasonic, 5V | 3 | J_US1/2/3 | Echo goes through on-board dividers |
| Raspberry Pi 4 | — | 1 | J_PI_PWR + J_UART | Powered from 5V rail; UART for commands |
| Pi Camera v2 | CSI | 1 | (Pi CSI port) | Not on this PCB |
| USB speaker | small amplified | 1 | (Pi USB) | Audio is Pi-side; not on this PCB |
| 2S LiPo 2200mAh | 7.4V, XT30 | 1 | J1 | The 2S — NOT the 4S |

---

## Passives

| Ref | Value | Package | Qty | Purpose |
|---|---|---|---|---|
| C1 | 10µF | 0805 / elec | 1 | ESP32 3V3 bulk decoupling |
| C2 | 100nF | 0603 | 1 | ESP32 3V3 HF decoupling (close to pin) |
| C3, C4 | 10µF | elec | 2 | AMS1117 input + output |
| C5, C6 | 100nF | 0603 | 2 | AMS1117 input + output HF |
| C7 | 1000µF | elec (low ESR) | 1 | MP1584EN 5V output bulk (Pi inrush) |
| C8 | 100nF | 0603 | 1 | MP1584EN 5V output HF |
| C9 | 10µF | ceramic (X5R/X7R) | 1 | At XT30 input — fast transient response (lock item C2) |
| C10 | 100µF | elec | 1 | VBAT input bulk |
| C11, C12 | 470µF | elec (low ESR) | 2 | One at each TB6612FNG VM pin (within 3mm) |
| C13, C14 | 100nF | 0603 | 2 | TB6612FNG VCC decoupling (one per chip) |
| C15, C16 | 100nF | 0603 | 2 | Encoder signal line filtering (failure mode #9) |
| C17 | 1µF | 0603 | 1 | ESP32 EN pin cap (power-on delay) |
| C18 | 100nF | 0603 | 1 | Battery-sense ADC node filter |
| R1, R2, R3 | 1kΩ | 0603 | 3 | HC-SR04 echo divider — series (5V→3.33V) |
| R4, R5, R6 | 2kΩ | 0603 | 3 | HC-SR04 echo divider — to GND |
| R7, R8 | 4.7kΩ | 0603 | 2 | I2C SDA/SCL pull-ups to 3V3 (serves IMU + both OLEDs) |
| R9 | 10kΩ | 0603 | 1 | ESP32 EN pull-up to 3V3 |
| R10 | 100Ω | 0603 | 1 | Status LED current limit |
| R11 | 10kΩ | 0603 | 1 | GPIO0 pull-up to 3V3 (boot) |
| R12 | 10kΩ | 0603 | 1 | Battery sense divider — VBAT side |
| R13 | 3.3kΩ | 0603 | 1 | Battery sense divider — GND side |
| R14, R15 | 10kΩ | 0603 | 2 | STBY pull-ups (or tie STBY directly to 3V3 — see schematic) |

> If U2 is the bare MP1584EN IC instead of a module, add: L (e.g., 10µH 3A), D (Schottky, e.g., SS34), and feedback resistors R_fb1=100k / R_fb2≈19.1k for 5.0V (Vout = 0.8×(1+R_fb1/R_fb2)). **The module is recommended for a first board** to avoid switching-regulator layout pitfalls.

---

## Protection

| Ref | Part | Value | Qty | Notes |
|---|---|---|---|---|
| F1 | Polyfuse (PPTC) | 7A hold | 1 | Littelfuse RGEF700 or equiv. **7A, not 5A** — avoids pre-trip voltage sag (lock item A5) |

---

## Connectors

| Ref | Type | Pins | Qty | Connects |
|---|---|---|---|---|
| J1 | XT30 | 2 | 1 | Battery input (VBAT, GND) |
| J_PI_PWR | Header/JST | 2 | 1 | 5V + GND to Pi GPIO pins 2 & 6 |
| J_UART | Header/JST | 3 | 1 | ESP32 TX→Pi RX, ESP32 RX←Pi TX, GND |
| J_PROG | Header | 6 | 1 | Programming: 3V3, GND, EN, IO0, TXD0, RXD0 |
| J_ML_F, J_ML_R | 2-pin | 2 | 2 | Left-front, left-rear motor power |
| J_MR_F, J_MR_R | 2-pin | 2 | 2 | Right-front, right-rear motor power |
| J_ENC_L, J_ENC_R | 4-pin | 4 | 2 | One encoder per side: VCC, GND, out (A) — read on GPIO34/35 |
| J_US1, J_US2, J_US3 | 4-pin | 4 | 3 | HC-SR04 center / left / right (VCC, Trig, Echo, GND) |
| J_MAST | 4-pin | 4 | 1 | I2C to mast: 3V3, GND, SDA, SCL → both OLED eyes |
| SW1 | Tactile | 2 | 1 | BOOT (GPIO0 → GND) |
| SW2 | Tactile | 2 | 1 | RESET (EN → GND) |

---

## Power Rails Summary

| Rail | Source | Feeds |
|---|---|---|
| VBAT (6.4–8.4V) | J1 → F1 → Q1 | TB6612FNG VM (motors), MP1584EN input, battery-sense divider |
| 5V | MP1584EN (U2) | Raspberry Pi, AMS1117 input, HC-SR04 VCC, (optional speaker) |
| 3.3V | AMS1117 (U3) | ESP32, MPU-6050, TB6612FNG VCC/STBY, OLED eyes, I2C pull-ups, LED |

**Key locked decisions reflected here:** AMS1117 fed from 5V (not VBAT); 7A polyfuse; one H-bridge channel per motor; OLED eyes on existing I2C; audio off-board on the Pi.
