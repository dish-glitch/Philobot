# Philo — Bill of Materials (Complete)

**Status: Schematic complete, ERC clean, all footprints assigned. Ready for PCB layout.**

This is the authoritative parts list matching the annotated KiCad schematic. Reference designators reflect post-annotation numbering.

---

## Active Components

| Ref | Part | Value / Spec | Footprint | Qty | Notes |
|---|---|---|---|---|---|
| U8 | ESP32-WROOM-32 | 240MHz, 4MB flash | RF_Module:ESP32-WROOM-32 | 1 | Castellated SMD module |
| J8 | Pololu D24V50F5 | 5.0V fixed out, 5A buck | PinHeader_1x05_P2.54mm | 1 | Complete module — no external inductor/feedback needed. VIN←VBAT, VOUT→+5V |
| U5 | AMS1117-3.3 | 3.3V LDO, 800mA | SOT-223-3_TabPin2 | 1 | Input from **+5V rail**, NOT VBAT — thermal lock decision |
| U6 | TB6612FNG | Dual H-bridge, 1.2A/ch | SSOP-24_5.3x8.2mm_P0.65mm | 1 | Left-front + right-front motors |
| U7 | TB6612FNG | Dual H-bridge, 1.2A/ch | SSOP-24_5.3x8.2mm_P0.65mm | 1 | Left-rear + right-rear motors |
| U1 | MPU-6050 (GY-521 module) | 6-axis IMU, I2C | PinHeader_1x08_P2.54mm | 1 | Address 0x68 (AD0→GND). Use GY-521 breakout |
| Q2 | AOD4185 P-channel MOSFET | Vds>30V, 10A | TO-252-3_TabPin2 | 1 | Reverse-polarity protection. Drain=battery side, Source=VBAT rail |
| D1 | Status LED | Any color | LED_0805_2012Metric | 1 | GPIO2 → R11 (100Ω) → LED → GND |

---

## Off-Board Modules (connect via headers)

| Part | Spec | Qty | Connector | Est. Price | Notes |
|---|---|---|---|---|---|
| OLED display | SSD1306, 128x64, I2C, 0.96 inch | 2 | J4 (J_MAST) | ~$4 ea (Amazon) | One at 0x3C (telemetry / left eye), one at 0x3D (right eye). Set address via solder jumper on back of module. Mounted on camera mast as eyes. **Buy 2.** |
| HC-SR04 | Ultrasonic, 5V | 3 | J1/J2/J3 | Echo lines go through on-board 1k/2k dividers |
| Raspberry Pi 4 | — | 1 | J5 + J6 | 5V from rail; UART for commands |
| Pi Camera v2 | CSI | 1 | Pi CSI port | Not on this PCB |
| USB speaker | Small amplified | 1 | Pi USB | Audio is Pi-side only |
| 2S LiPo 2200mAh | 7.4V nominal, XT30 plug | 1 | J7 (XT30) | **2S only — NOT 4S** |
| JGA25-370 motors | 6V, 200RPM, 4mm D-shaft | 4 | J9/J10/J11/J12 | 2 per TB6612FNG |
| Encoders | One per side | 2 | J13/J14 | GPIO34 (left), GPIO35 (right) |

---

## Passives

| Ref | Value | Footprint | Qty | Purpose |
|---|---|---|---|---|
| C1,C2,C3,C18,C20,C21,C25,C26,C28,C31 | 100nF | C_0603_1608Metric | 10 | Decoupling / HF bypass |
| C17,C19,C23,C32 | 10µF | C_0805_2012Metric | 4 | Bulk ceramic decoupling |
| C30 | 1µF | C_0603_1608Metric | 1 | ESP32 EN pin delay cap |
| C24 | 100µF electrolytic | CP_Radial_D6.3mm_P2.50mm | 1 | VBAT input bulk |
| C27,C29 | 470µF electrolytic (low ESR) | CP_Radial_D10.0mm_P5.00mm | 2 | TB6612FNG VM bulk (one per chip, within 3mm) |
| C22 | 1000µF electrolytic (low ESR) | CP_Radial_D12.5mm_P5.00mm | 1 | D24V50F5 5V output bulk (Pi inrush) |
| R1,R3,R8 | 1kΩ | R_0603_1608Metric | 3 | HC-SR04 echo divider — series |
| R2,R7,R9 | 2kΩ | R_0603_1608Metric | 3 | HC-SR04 echo divider — to GND |
| R4,R5,R10,R12 | 10kΩ | R_0603_1608Metric | 4 | Gate resistor (Q2), EN pull-up, GPIO0 pull-up, VBAT sense |
| R6 | 3.3kΩ | R_0603_1608Metric | 1 | Battery sense divider — GND side |
| R11 | 100Ω | R_0603_1608Metric | 1 | Status LED current limit |
| R13,R14 | 4.7kΩ | R_0603_1608Metric | 2 | I2C SDA/SCL pull-ups (shared by IMU + both OLEDs) |

---

## Protection & Switches

| Ref | Part | Value | Footprint | Qty | Notes |
|---|---|---|---|---|---|
| F2 | Polyfuse PPTC | 7A hold | CP_Radial_D10.0mm_P5.00mm | 1 | Littelfuse RGEF700. **7A not 5A** — avoids pre-trip sag |
| SW1 | Tactile switch | — | SW_PUSH_6mm (THT) | 1 | RESET — EN → GND |
| SW2 | Tactile switch | — | SW_PUSH_6mm (THT) | 1 | BOOT — GPIO0 → GND |
| TP1–TP6 | Test point | — | TestPoint_Pad_D1.5mm | 6 | VBAT, +5V, +3V3, GND×2, ESP_TX, ESP_RX |

---

## Connectors

| Ref | Value | Footprint | Qty | Connects |
|---|---|---|---|---|
| J7 | XT30 battery | AMASS_XT30PW-F Horizontal | 1 | Battery +/− input |
| J8 | D24V50F5 module | PinHeader_1x05_P2.54mm | 1 | Buck regulator (VIN, GND, VOUT, EN, PG) |
| J1,J2,J3 | HC-SR04 center/left/right | PinHeader_1x04_P2.54mm | 3 | Ultrasonic sensors |
| J4 | J_MAST | PinHeader_1x04_P2.54mm | 1 | I2C mast: +3V3, GND, SDA, SCL |
| J5 | J_PI_PWR | PinHeader_1x02_P2.54mm | 1 | +5V + GND to Pi |
| J6 | J_UART | PinHeader_1x03_P2.54mm | 1 | ESP_TX, ESP_RX, GND to Pi |
| J9,J10 | J_ML_F, J_MR_F | PinHeader_1x02_P2.54mm | 2 | Front motor outputs (U6) |
| J11,J12 | J_ML_R, J_MR_R | PinHeader_1x02_P2.54mm | 2 | Rear motor outputs (U7) |
| J13,J14 | ENC_L, ENC_R | PinHeader_1x03_P2.54mm | 2 | Encoders: +3V3, GND, signal |

---

## Power Rails Summary

| Rail | Source | Feeds |
|---|---|---|
| VBAT (6.4–8.4V) | J7 → F2 → Q2 | TB6612FNG VM, D24V50F5 input, battery-sense divider |
| +5V (5.0V, 5A) | D24V50F5 (J8) | Raspberry Pi, AMS1117 input, HC-SR04 VCC |
| +3V3 (3.3V, 800mA) | AMS1117 (U5) | ESP32, MPU-6050, TB6612FNG VCC/STBY, OLEDs, pull-ups, LED |

**Locked decisions:** AMS1117 fed from +5V not VBAT (thermal); 7A polyfuse; one motor per H-bridge channel; OLEDs on existing I2C bus; audio off-board on Pi.
