# Hardware — Philo PCB

**Owner: [@dish-glitch](https://github.com/dish-glitch)**

---

## Overview

The PCB is the central hub of Philo. Everything connects to it — motors, sensors, battery, Raspberry Pi, and the ESP32 that runs the firmware. Designed in KiCad, fabricated at JLCPCB.

---

## PCB Blocks

### 1. Microcontroller — ESP32-WROOM-32
The brain. Handles all motor control, sensor reading, and serial communication with the Raspberry Pi. Connects to the PCB via castellated edge pads. Programmed over USB via a CP2102 breakout (not soldered to the board — plugs into a 6-pin header).

Key ESP32 pins in use:
- 4x PWM outputs (motor speed, one per motor pair side × 2, plus direction pins)
- 6x GPIO for ultrasonic sensors (trigger + echo × 3)
- 2x I2C (SDA, SCL) for IMU
- 2x UART (TX, RX) for Raspberry Pi communication
- 2x encoder inputs per side (4 total, interrupt-capable pins)

### 2. Motor Drivers — TB6612FNG × 2
Each TB6612FNG drives 2 motors. Two chips = 4 motors total.

- Input: PWM + direction signals from ESP32
- Output: 1.2A continuous per channel, 3A peak
- Logic voltage: 3.3V (directly compatible with ESP32, no level shifting)
- Motor voltage: up to 13.5V (runs directly from 2S LiPo at 8.4V max)
- No heatsink needed at these current levels for a 600g robot

Place motor driver chips close to the motor output connectors to keep high-current traces short.

**Why TB6612FNG and not L298N:** The L298N is an older bipolar transistor design that drops ~2V across its internal switches at 1A, wasting that energy as heat and requiring a heatsink. At 8.4V battery with 2V drop, you are feeding your motors 6.4V instead of 8.4V — 24% speed loss. The TB6612FNG is MOSFET-based with near-zero voltage drop. Use the right part.

### 3. Power Architecture

**Input:** 2S LiPo, 6.4V discharged to 8.4V fully charged.
**Connector:** XT30 (rated 30A continuous — massively oversized for us, which means it will never be the failure point).
**Protection:** 5A polyfuse on battery input. Resets after cooldown. Protects against shorts during development without requiring fuse replacement.

**3.3V rail — AMS1117-3.3 LDO:**
Powers ESP32, IMU, and all 3.3V logic.
- Output: 3.3V at up to 800mA
- ESP32 draws ~240mA average under load
- Add 10µF + 100nF caps on both input and output pins — do not skip this, the LDO will oscillate without them

**5V rail — MP1584EN buck converter:**
Powers Raspberry Pi via GPIO header (pin 2 = 5V, pin 6 = GND).

> **Critical:** The original spec said MP2307. Do not use it. The MP2307 is rated 1.2A max. A Raspberry Pi 4 running YOLOv8 inference draws up to 3A. The MP2307 would overheat, throttle, and crash the Pi mid-demo. **Use the MP1584EN — it is rated 3A, costs ~$1-2, and fits in nearly the same footprint.**

- Input: battery voltage (6.4–8.4V)
- Output: 5V at 3A
- Add 100µF bulk capacitor on the output — the Pi has inrush current spikes when its CPU ramps up
- Efficiency: ~90% at this conversion ratio (far better than an LDO which would burn ~40% as heat)

### 4. IMU — MPU-6050
Connected via I2C (SDA/SCL). Reads pitch angle to detect if Philo is tipping over.

- If pitch exceeds ±30 degrees, firmware cuts all motor power
- Mount the IMU flat and level on the PCB
- The accelerometer is sensitive to which axis is forward — document axis orientation in firmware

### 5. Ultrasonic Sensors — HC-SR04 × 3

Not soldered to the PCB — they mount to the chassis and connect via headers. Each needs one trigger pin and one echo pin on the ESP32.

**Placement on chassis:**
- Front-left: angled ~30 degrees left
- Front-center: straight ahead
- Front-right: angled ~30 degrees right

This triangle gives Philo awareness of what is directly ahead and to each side. All three readings together determine whether to stop, bias left, or bias right.

**HC-SR04 specs:**
- Range: 2cm to 400cm
- Trigger: 10µs HIGH pulse
- Echo: pulse width proportional to distance (58µs per cm)
- Operating voltage: 5V

**Voltage divider required on every echo line:**

The HC-SR04 echo pin outputs 5V logic. The ESP32 GPIO max input is 3.3V. Feeding 5V into an ESP32 pin will damage it over time and may kill it immediately.

Fix: resistor voltage divider on each echo line.

```
HC-SR04 echo ──┬── 1kΩ ──── ESP32 GPIO
               │
              2kΩ
               │
              GND
```

Output voltage = 5V × (2kΩ / (1kΩ + 2kΩ)) = 3.33V ✓

Add this divider to all 3 echo lines on the PCB. The trigger lines are outputs from the ESP32 (3.3V) — HC-SR04 trigger accepts 3.3V, no level shifting needed there.

### 6. Connectors

| Connector | Type | Purpose |
|---|---|---|
| Battery input | XT30 male | 2S LiPo |
| Motor outputs × 4 | 2-pin JST PH 2.0 | One per motor |
| Encoder inputs × 4 | 3-pin header (VCC, GND, signal) | One per motor |
| Ultrasonic × 3 | 4-pin header (VCC, GND, trigger, echo) | One per sensor |
| UART to RPi | 4-pin header (TX, RX, GND, 3.3V) | Serial command link |
| RPi 5V power | 2-pin header | 5V + GND to Pi GPIO pin 2 and 6 |
| IMU | 4-pin header (VCC, GND, SDA, SCL) | MPU-6050 |
| Programming | 6-pin header (TX, RX, EN, GPIO0, 3.3V, GND) | CP2102 programmer |
| Status LEDs × 2 | On-board SMD | Power always-on, status firmware-controlled |
| Reset button | Tactile switch (6mm) | ESP32 EN pin |

---

## Power Budget

Understanding how much current everything draws is important for sizing the battery, the buck converter, and the PCB traces.

| Component | Voltage | Current (typical) | Power |
|---|---|---|---|
| Raspberry Pi 4 (under inference load) | 5V | 2.5–3A | 12.5–15W |
| 4x N20 motors (running, not stalled) | 8.4V | ~200mA each = 800mA total | ~6.7W |
| ESP32 (active, WiFi off) | 3.3V | ~240mA | ~0.8W |
| 3x HC-SR04 | 5V | ~15mA each = 45mA | ~0.2W |
| MPU-6050 | 3.3V | ~3.9mA | negligible |
| Status LEDs × 2 | 3.3V | ~20mA | negligible |
| **Total system** | | | **~23–25W** |

**What this means for the battery:**

2S LiPo 2200mAh at 7.4V nominal = 16.3 Wh of stored energy.

```
Runtime estimate = 16.3 Wh / 25W = ~0.65 hours = ~39 minutes
```

40 minutes of runtime is plenty for the 60-second demo video and all the test runs leading up to it. A 3000mAh pack bumps runtime to ~53 minutes if needed.

**Motor stall warning:** If a motor stalls (wheel jammed against a wall), current jumps from ~200mA to 800–1000mA per motor. With 4 motors potentially stalling: 4A from the battery on the motor side alone. The polyfuse is rated 5A and will trip. This is intentional — it protects the motor drivers. The corner escape behavior in firmware should prevent sustained stalling.

---

## Trace Width Requirements

Motor current carries through PCB copper traces. Undersized traces will heat up, resistance will increase, voltage will drop, and in worst cases the trace lifts or burns.

| Net | Current | Minimum Trace Width |
|---|---|---|
| Battery input (VBAT) | up to 4A | 2.0mm |
| Motor outputs | up to 1.2A continuous | 1.0mm |
| 5V RPi rail | up to 3A | 1.5mm |
| 3.3V logic rail | ~500mA | 0.5mm |
| Signal traces (UART, I2C, GPIO) | <50mA | 0.2mm (default) |

Use the IPC-2221 trace width calculator or the KiCad built-in tool to verify. Do not guess.

---

## PCB Design Checklist

- [ ] ESP32-WROOM-32 placed with 100nF decoupling cap on VCC
- [ ] TB6612FNG x2 placed near motor output connectors
- [ ] AMS1117-3.3 with 10µF + 100nF on input and output
- [ ] MP1584EN buck converter (3A rated) with 100µF output cap
- [ ] XT30 battery connector with 5A polyfuse
- [ ] Voltage dividers on all 3 HC-SR04 echo lines (1kΩ + 2kΩ)
- [ ] MPU-6050 header (4-pin I2C)
- [ ] 3x ultrasonic sensor headers (4-pin)
- [ ] 4x motor output connectors (2-pin JST PH)
- [ ] 4x encoder input headers (3-pin)
- [ ] UART header for RPi (4-pin)
- [ ] RPi 5V power header (2-pin)
- [ ] Programming header (6-pin)
- [ ] Power LED + status LED with current limiting resistors (~100Ω for 3.3V)
- [ ] Reset button on ESP32 EN pin with 10kΩ pull-up to 3.3V
- [ ] Ground plane poured on both layers
- [ ] Motor current traces ≥ 1.0mm
- [ ] Battery input traces ≥ 2.0mm
- [ ] 5V RPi rail traces ≥ 1.5mm
- [ ] DRC passes clean — zero errors
- [ ] Gerbers exported and visually verified in Gerber viewer before uploading

---

## Timeline

| Week | Target |
|---|---|
| 1–2 | Schematic 80% done — ESP32, motor drivers, power block, all headers placed |
| 2–3 | Schematic complete, ERC clean. PCB layout started. |
| 3 | Layout complete, DRC clean, Gerbers to JLCPCB. Order 5 boards. |
| 4–5 | PCB arrives (~10 days). Solder and power-on test. |
| 5 | Motor spin confirmed over serial command. |
