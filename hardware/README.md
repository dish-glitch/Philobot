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
- Output: up to 1.2A continuous per motor channel (3A peak)
- Logic voltage: 3.3V (compatible with ESP32 direct)
- Motor voltage: up to 13.5V (runs directly from 2S LiPo at 8.4V max)
- No heatsink needed at these current levels

Place motor driver chips close to the motor output connectors to keep high-current traces short.

### 3. Power — 2S LiPo Input
- Input: 6.4V (discharged) to 8.4V (fully charged) from 2S LiPo
- Connector: XT30 (rated 30A, compact, standard for small robotics)
- Inline polyfuse: 5A — protects against shorts during development
- Reverse polarity protection: P-channel MOSFET or Schottky diode on battery input

**3.3V rail:** AMS1117-3.3 LDO. Powers ESP32 and all logic. Add 10µF + 100nF caps on input and output.

**5V rail:** MP2307 or similar buck converter. Powers Raspberry Pi via GPIO pin 2 (5V) and pin 6 (GND). Must supply at least 2.5A — the Pi 4 can draw up to 3A under load. Add 100µF bulk cap on output.

### 4. IMU — MPU-6050
Connected via I2C (SDA/SCL). Detects tilt angle. If Philo tips forward going too fast, the firmware reads the pitch angle and cuts motor speed. Prevents the robot from nosediving.

Mount the IMU flat and level on the PCB. The accelerometer is sensitive to board orientation — document which axis is forward in firmware comments.

### 5. Ultrasonic Sensors — HC-SR04 × 3
Not soldered to the PCB — they mount to the chassis and connect via 3-pin headers (VCC, GND, signal). Each sensor needs one trigger pin and one echo pin on the ESP32.

**Placement on chassis (mechanical team confirms exact angles):**
- Front-center: straight ahead
- Front-left: angled ~30 degrees left
- Front-right: angled ~30 degrees right

This triangle of sensors gives Philo awareness of what is directly in front and slightly to each side. The firmware uses all three readings together to decide whether to stop or which way to steer around an obstacle.

**HC-SR04 specs:**
- Range: 2cm to 400cm
- Trigger: 10µs HIGH pulse on trigger pin
- Echo: pulse width proportional to distance (148µs per inch)
- Operating voltage: 5V — use a voltage divider on the echo line (echo output is 5V, ESP32 GPIO is 3.3V max)

Voltage divider on echo pin: 1kΩ from sensor echo to ESP32 GPIO, 2kΩ from that node to GND. This brings 5V down to 3.3V safely.

### 6. Connectors

| Connector | Type | Purpose |
|---|---|---|
| Battery in | XT30 male | 2S LiPo |
| Motor outputs (x4) | 2-pin JST PH 2.0 | One per motor |
| Encoder inputs (x4) | 3-pin header | VCC, GND, signal per motor |
| Ultrasonic (x3) | 3-pin header | VCC, GND, signal (trigger+echo combined via firmware timing) — or 4-pin if trigger and echo are separate |
| UART to RPi | 4-pin header | TX, RX, GND, 3.3V |
| IMU | 4-pin header | VCC, GND, SDA, SCL |
| 5V RPi power | 2-pin | 5V out to Pi GPIO |
| Programming header | 6-pin | TX, RX, EN, GPIO0, 3.3V, GND |
| Status LEDs | — | Power LED always on, status LED firmware controlled |
| Reset button | Tactile switch | ESP32 EN pin |

---

## Obstacle Avoidance Approach

The ultrasonic sensors feed into a priority system in firmware. The Raspberry Pi sends movement commands, but the ESP32 can override any command if sensors detect an obstacle.

**Decision logic (firmware handles this, documented here for reference):**

```
if front_center < 25cm:
    stop, back up, turn toward open side

elif front_left < 30cm:
    slow left motor, bias right

elif front_right < 30cm:
    slow right motor, bias left

else:
    execute Pi command normally
```

This is called a priority layer — safety beats navigation. The Pi does not know about obstacles. The ESP32 handles it independently. This separation keeps the vision code simple and keeps obstacle response fast (no round trip to the Pi).

The approach is sometimes called potential fields — obstacles push the robot away, the person pulls it forward. The net result is that Philo curves around things rather than stopping dead and jerking.

**Corner escape behavior:** If all three sensors trigger simultaneously (robot is cornered), the firmware runs a fixed escape sequence: back up 300ms, spin 90 degrees, then re-scan. Without this, the robot would freeze indefinitely.

---

## PCB Design Checklist

- [ ] ESP32-WROOM-32 placed and decoupled (100nF on VCC)
- [ ] TB6612FNG x2 placed near motor connectors
- [ ] AMS1117-3.3 with 10µF + 100nF caps
- [ ] MP2307 buck converter with 100µF output cap
- [ ] XT30 battery connector with polyfuse
- [ ] MPU-6050 header (I2C)
- [ ] 3x ultrasonic sensor headers (4-pin each)
- [ ] 4x motor output connectors (2-pin JST)
- [ ] 4x encoder input headers (3-pin)
- [ ] UART header for RPi (4-pin)
- [ ] Programming header (6-pin)
- [ ] Power LED + status LED with current limiting resistors
- [ ] Reset button on ESP32 EN pin
- [ ] Voltage dividers on HC-SR04 echo lines
- [ ] Ground plane poured on both layers
- [ ] Motor current traces ≥ 1mm wide
- [ ] Power rail traces ≥ 2mm wide
- [ ] DRC passes clean
- [ ] Gerbers exported and verified

---

## Timeline

| Week | Target |
|---|---|
| 1-2 | Schematic 80% done — ESP32, motor drivers, power block placed |
| 2-3 | Schematic complete, ERC clean. Layout started. |
| 3 | Layout complete, DRC clean, Gerbers to JLCPCB |
| 4-5 | PCB arrives, solder, power-on test |
| 5 | Motor spin confirmed over serial command |
