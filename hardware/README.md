# Hardware — Philo PCB

**Owner: [@dish-glitch](https://github.com/dish-glitch)**

---

## Overview

The PCB is the central hub of Philo. Everything connects here — motors, sensors, battery, Raspberry Pi, ESP32 firmware. Designed in KiCad, fabricated at JLCPCB (5-board run, ~10 day lead time including shipping).

---

## Component Specifications (Verified from Datasheets)

Every part below has been cross-referenced against its datasheet or product page. Do not substitute components without checking if the replacement meets the same electrical requirements.

| Component | Key Specs | Source | Weight |
|---|---|---|---|
| ESP32-WROOM-32 | 240MHz dual-core, 520KB SRAM, 3.3V, 18×25.5×3.1mm | [Espressif datasheet](https://www.espressif.com/sites/default/files/documentation/esp32-wroom-32d_esp32-wroom-32u_datasheet_en.pdf) | 2.5g |
| TB6612FNG × 2 | 1.2A continuous / 3.2A peak per channel, 4.5–13.5V motor, 2.7–5.5V logic, 100kHz PWM max | [Toshiba datasheet](https://cdn-shop.adafruit.com/datasheets/TB6612FNG_datasheet_en_20121101.pdf) | 1.5g each |
| MPU-6050 (GY-521 module) | 3-axis accel ±16g, 3-axis gyro ±2000°/s, I2C, 3.3–5V, 21×15mm | [InvenSense datasheet](https://product.tdk.com/en/search/sensor/mortion-inertial/imu/info?part_no=MPU-6050) | 13g |
| HC-SR04 × 3 | 5V, <15mA, 2–400cm range, 40kHz, 45×20×15mm | [Sparkfun datasheet](https://cdn.sparkfun.com/datasheets/Sensors/Proximity/HCSR04.pdf) | 10g each |
| AMS1117-3.3 LDO | 3.3V out, 800mA max, SOT-223 package | AMS datasheet | <1g |
| MP1584EN Buck Converter | 4.5–28V in, adjustable out, **3A continuous**, 1.5MHz switching, SOP-8 | MPS datasheet | <1g |
| 2S LiPo 2200mAh | 7.4V nominal, 8.4V full, 6.4V cutoff, 97–130g depending on model | Gens Ace product page | ~110g |
| XT30 Connector | 30A continuous, gold plated, compact | XT30 spec | 3g per pair |

---

## PCB Block Architecture

### Block 1 — Microcontroller: ESP32-WROOM-32

The ESP32 is the only firmware-running chip on this board. It handles everything: motor PWM, PID loops, serial comms with the Pi, ultrasonic polling, and IMU reading.

The module solders to castellated pads on the PCB edge. This is a standard footprint — use the official KiCad footprint from the Espressif KiCad library.

**Decoupling:** Place one 100nF ceramic cap and one 10µF electrolytic cap between VCC (pin 1) and GND as close to the module as the layout permits. Without these, the module can brownout during WiFi transmit spikes even if WiFi is not used in firmware.

**Programming:** The ESP32 is flashed via a 6-pin header (TX, RX, EN, GPIO0, 3.3V, GND). A CP2102 USB-UART module plugs into this header for programming — it is not soldered to the board. This keeps the PCB light and avoids having a USB connector at board revision 1.

To enter bootloader: pull GPIO0 LOW before reset. Add a small tactile button from GPIO0 to GND on the board for this.

### Block 2 — Motor Drivers: TB6612FNG × 2

Two TB6612FNG ICs. Each drives two motors. Total: 4 motors driven.

**Why not L298N:** The L298N uses bipolar transistors with a 2V forward voltage drop per switch. At 8.4V battery input, motor sees only 6.4V (24% speed loss) and the L298N dissipates 2V × 1.2A = 2.4W as heat per channel requiring a heatsink. The TB6612FNG uses MOSFETs with a near-zero drop. No heatsink. Full voltage to motors.

**Wiring each TB6612FNG:**
- PWMA pin → ESP32 GPIO (PWM output)
- AIN1, AIN2 → ESP32 GPIO (direction control)
- AO1, AO2 → Left-front motor terminals
- PWMB, BIN1, BIN2, BO1, BO2 → Same for left-rear motor
- STBY pin → 3.3V (HIGH = enabled, LOW = all outputs off). Tie HIGH permanently.
- VM → Battery voltage (VBAT, up to 13.5V)
- VCC → 3.3V logic

**Placement:** Place both TB6612FNG ICs on the same side of the board as the motor output connectors. Keep AO1/AO2 and BO1/BO2 traces short and wide (≥1.0mm). Motor current of 1.2A through a thin 0.25mm trace will heat the copper and increase resistance.

### Block 3 — Power Architecture

This is the most critical block to get right. Power failures mid-demo are catastrophic.

**Input: 2S LiPo via XT30**

Battery voltage range: 6.4V (discharged) to 8.4V (full charge). Connect through a 5A polyfuse (rated fuse current, not trip current — it trips at ~2× rating under sustained fault). The polyfuse resets after cooldown; no need to replace a blown fuse during development.

Add a P-channel MOSFET for reverse polarity protection on the battery input. If the battery is connected backward (easy mistake with XT30 if using a non-standard connector), the MOSFET blocks current instead of frying every IC on the board.

**3.3V Rail: AMS1117-3.3 LDO**

Supplies: ESP32, MPU-6050, logic side of TB6612FNG, all signal-level circuits.

Input cap: 10µF electrolytic + 100nF ceramic directly on input pin.
Output cap: 10µF electrolytic + 100nF ceramic directly on output pin.
Do not omit these. The AMS1117 datasheet specifies them for stability. Without them the output oscillates.

Maximum load on 3.3V rail: ESP32 (~240mA) + MPU-6050 (~4mA) + TB6612FNG logic × 2 (~2mA each) + LEDs (~20mA) ≈ 268mA. AMS1117-3.3 is rated 800mA. Adequate margin.

**5V Rail: MP1584EN Buck Converter**

> **Critical correction from earlier revision:** The MP2307 was originally specified here. The MP2307 is rated 1.2A maximum. A Raspberry Pi 4 running YOLOv8 inference continuously draws up to 2.5–3A. The MP2307 would overheat within minutes and crash the Pi. The MP1584EN is rated 3A continuous and is the correct part.

Input: VBAT (6.4–8.4V)
Output: Set to exactly 5.0V via resistor divider on feedback pin (per MP1584EN datasheet, formula provided in section 7 of datasheet)
Output cap: 100µF electrolytic + 100nF ceramic. The Pi 4 has significant inrush current when its CPU ramps under load. The 100µF cap absorbs the spike and prevents the buck converter from dropping out.

The 5V output connects to the Raspberry Pi via a 2-pin header that plugs into Pi GPIO pin 2 (5V) and pin 6 (GND). Do not use the Pi's USB-C port for power — that requires a cable run outside the chassis and is mechanically fragile in a mobile robot.

**Power sequencing:** The 3.3V rail from the LDO and the 5V rail from the buck converter both come up approximately together when the battery is connected. The ESP32 boots in ~300ms. The Pi takes ~20–30 seconds to boot. The ESP32 firmware must not send any motor commands until it has received at least one valid command from the Pi. Default state on boot: all motors stopped, watchdog timer running.

### Block 4 — IMU: MPU-6050

Connects via I2C (SDA/SCL). Default I2C address: 0x68 (AD0 pin LOW). If a second I2C device at 0x68 is ever added, there will be a bus collision. Document the I2C address map.

Mount the GY-521 module on a 4-pin header at the PCB level (flat and horizontal). The accelerometer measures gravity vector — if the module is tilted or mounted at an angle, firmware must apply an offset correction. Flat mount avoids this.

Add 4.7kΩ pull-up resistors from SDA and SCL to 3.3V on the PCB. The ESP32 has internal pull-ups but they are weak and unreliable at the I2C frequencies we use.

### Block 5 — Ultrasonic Sensors: HC-SR04 × 3

The sensors mount on the chassis (not the PCB) and connect via 4-pin headers: VCC (5V), GND, Trigger (3.3V out from ESP32), Echo (5V out from sensor — NEEDS LEVEL SHIFTING).

**Voltage level issue on Echo pin — this will damage the ESP32 without mitigation:**

The HC-SR04 outputs 5V logic on its Echo pin. The ESP32's GPIO absolute maximum input voltage is 3.6V. Sustained 5V input will degrade the GPIO over time and can cause immediate damage.

Fix — resistor voltage divider on each Echo line:

```
HC-SR04 Echo (5V) ──── 1kΩ ──┬──── ESP32 GPIO (reads 3.33V) ✓
                              │
                             2kΩ
                              │
                             GND
```

V_out = 5V × 2kΩ/(1kΩ+2kΩ) = 3.33V

Add this divider for all 3 Echo lines on the PCB. 6 resistors total (1kΩ + 2kΩ per sensor). The Trigger lines from ESP32 (3.3V) are accepted by HC-SR04 as valid logic HIGH — no level shifting needed on trigger.

### Block 6 — Connectors

| Connector | Spec | Purpose |
|---|---|---|
| Battery | XT30 male | 2S LiPo input |
| Motor outputs × 4 | 2-pin JST PH 2.0 | One per motor (polarity matters — document which pin is positive) |
| Encoder inputs × 4 | 3-pin header (VCC, GND, Signal) | One per motor — signals are 3.3V level from encoder, ESP32 interrupt pins |
| Ultrasonic × 3 | 4-pin header (5V, GND, Trig, Echo) | One per sensor, Echo goes through divider on PCB |
| UART to RPi | 4-pin header (ESP TX, ESP RX, GND, 3.3V) | Cross-connect: ESP TX → RPi RX, ESP RX → RPi TX |
| 5V RPi power | 2-pin header | 5V and GND to Pi GPIO pins 2 and 6 |
| IMU | 4-pin header (3.3V, GND, SDA, SCL) | MPU-6050 module |
| Programming | 6-pin header (TX, RX, EN, GPIO0, 3.3V, GND) | CP2102 connects here for firmware upload |
| Status LEDs × 2 | On-board SMD 0805 | Power = always on; Status = firmware controlled |
| Reset button | 6mm tactile, through-hole | ESP32 EN pin with 10kΩ pull-up to 3.3V |

---

## Real Power Budget (Verified Numbers)

Understanding actual current draw is required to size the battery, the buck converter, and verify traces will not overheat.

| Component | Voltage Rail | Typical Current | Peak Current | Source |
|---|---|---|---|---|
| Raspberry Pi 4 (inference load) | 5V | 2.5A | 3.0A | Official Pi 4 power spec |
| 4× N20 motors (running at 45% safe torque) | VBAT | 4× ~120mA = 480mA | 4× 700mA = 2.8A (stall) | Extrapolated from stall spec |
| ESP32 (active, WiFi off) | 3.3V | 240mA | 340mA | Espressif datasheet |
| 3× HC-SR04 | 5V | 3× 15mA = 45mA | same | HC-SR04 datasheet |
| MPU-6050 | 3.3V | 3.9mA | same | InvenSense datasheet |
| LEDs, pull-ups, misc | 3.3V | ~20mA | same | Estimated |
| **Total normal operation** | | | **~3.3A from battery** | |
| **Total motor stall (worst case)** | | | **~5.8A from battery** | |

**Battery runtime at normal operation:**

2S LiPo 2200mAh (Gens Ace, 97g version):
- Nominal capacity: 2200mAh × 7.4V = 16.28 Wh
- Normal draw from battery: ~3.3A × 7.4V = 24.4W
- Runtime: 16.28 Wh / 24.4W ≈ **40 minutes**

40 minutes is sufficient for the 60-second demo video and all pre-demo test runs.

**Motor stall scenario current (all 4 motors stalled simultaneously):**
- 4 × 700mA = 2.8A motor stall current
- Plus Pi 3.0A + ESP32 0.24A = 6.04A total
- 5A polyfuse will trip at sustained ~5A. This is the correct behavior — if all motors are stalled, something is mechanically wrong and the fuse protects the PCB.

---

## Trace Width Requirements

PCB copper traces have resistance and thermal limits. Undersized traces heat up, increase in resistance, and eventually lift from the substrate or open.

| Net | Max Current | Required Trace Width | Notes |
|---|---|---|---|
| VBAT (battery to PCB) | 6A peak (stall) | **2.5mm minimum** | Size for fault, not normal operation |
| Motor outputs (AO1/AO2, BO1/BO2) | 1.2A continuous, 3.2A peak | **1.2mm minimum** | Both TB6612FNG outputs |
| 5V RPi rail | 3A | **1.5mm minimum** | Pi current at full inference load |
| 3.3V logic rail | 500mA | **0.5mm** | Use 0.8mm for margin |
| Signal traces (UART, I2C, GPIO) | <50mA | **0.25mm** (KiCad default) | Standard signal routing |

Use the KiCad interactive router with trace width rules set. Or verify with: [PCB Trace Width Calculator](https://www.digikey.com/en/resources/conversion-calculators/conversion-calculator-pcb-trace-width).

---

## Failure Mode Analysis

These are the hardware failures most likely to kill the project. Each has a mitigation built into the design.

### 1. Motor Burn from Sustained Stall
**What happens:** If a wheel is mechanically jammed (against a wall, under furniture), the motor draws its full 700mA stall current at 8.4V = 5.9W per motor. A micro N20 motor that size will overheat in under 2 minutes of sustained stall.
**Mitigation:** Polyfuse trips at ~5A sustained. ESP32 watchdog stops motors within 500ms of losing Pi communication. Ultrasonic obstacle avoidance triggers escape sequence before the robot reaches a sustained-stall situation. Do not test by forcing a wheel by hand for more than a few seconds.

### 2. RPi Crash from Underpowered 5V Rail
**What happens:** MP2307 (old spec) → 1.2A → Pi draws 2.5A → converter overheats → 5V rail collapses → Pi shuts down mid-demo.
**Mitigation:** MP1584EN, rated 3A. Verified against Pi 4 power requirements. 100µF output cap prevents brownout from inrush spikes.

### 3. ESP32 GPIO Damage from HC-SR04 5V Echo
**What happens:** HC-SR04 outputs 5V on Echo. ESP32 GPIO max 3.6V. Over time (or immediately in some cases) the GPIO input clamps internally, draws current, and either damages that pin or resets the chip.
**Mitigation:** 1kΩ / 2kΩ voltage divider on all 3 Echo lines. This is on the PCB schematic — verify it is there before sending Gerbers.

### 4. Battery Over-Discharge
**What happens:** 2S LiPo minimum cell voltage is 3.0V/cell = 6.0V total. If discharged below this, the battery's internal chemistry is permanently damaged. It will hold less charge and may swell (puffing = safety hazard).
**Mitigation:** Add a voltage divider from VBAT to an ESP32 ADC pin (10kΩ + 3.3kΩ divider to bring 8.4V max down to 2.1V, within ESP32 ADC range). Firmware monitors voltage and triggers low-battery cutoff at 6.4V (3.2V/cell — 0.2V above absolute minimum for safety margin). This also prevents the robot from dying mid-demo without warning — flash the status LED at low battery.

Divider calculation:
```
V_adc = V_bat × (3.3kΩ / (10kΩ + 3.3kΩ)) = V_bat × 0.248
At 6.4V battery: V_adc = 1.59V (within ESP32 ADC range of 0–3.3V)
At 8.4V battery: V_adc = 2.08V
```

### 5. Motor Wiring Polarity Error
**What happens:** If a motor connector is plugged in backward, that motor spins the wrong direction. "Forward" on that side becomes "backward." The robot spins in circles and cannot be corrected in software without swapping a connector.
**Mitigation:** Document motor wiring polarity in the PCB silkscreen (+ and − on each motor connector). Test each motor independently before full assembly by sending CMD:F,100 and verifying wheel spins forward (toward the front of the chassis). If wrong: swap the two motor wires on that connector, not in firmware.

### 6. I2C Bus Lockup
**What happens:** If the SDA line is stuck LOW (can happen if the Pi resets during an I2C transaction), the MPU-6050 holds SDA low indefinitely and the I2C bus is frozen. The ESP32 cannot recover without either a power cycle or a bus reset.
**Mitigation:** Implement I2C bus recovery in firmware (9 clock pulses on SCL while SDA is held — this is a standard recovery procedure in the I2C spec). Add this as an initialization routine that runs before the main loop.

### 7. Pi GPIO UART Garbage on Boot
**What happens:** During Raspberry Pi boot, the UART TX pin (GPIO14) is used by the kernel for boot messages. These appear as garbage bytes on the ESP32's UART RX. If the ESP32 tries to parse boot messages as motor commands, unexpected motor behavior can occur.
**Mitigation:** Do not process UART commands until a valid command structure is received (`CMD:` prefix). Invalid bytes are discarded. Also: Raspberry Pi OS on Lite image disables serial console by default (`raspi-config → Interface → Serial → disable console`). Do this during RPi setup — it is a required step.

### 8. PWM Noise on Encoder Signal Lines
**What happens:** Motor PWM at 20kHz can capacitively couple into nearby encoder signal lines (which are also routed near motors). This adds noise pulses to the encoder count, causing the PID loop to see phantom speed spikes and over/under-correct.
**Mitigation:** Route encoder traces on the opposite side of the PCB from motor traces. Add a 100nF ceramic cap from each encoder signal line to GND at the ESP32 GPIO pin. On the physical cable, twist encoder wires together (twisted pair reduces coupling).

### 9. 3D Print Shrinkage Causing PCB Mount Misalignment
**What happens:** PETG-CF shrinks ~0.5% on cooling. A 200mm chassis can shrink by ~1mm. If M3 standoff holes were designed for exact PCB corner positions, the PCB may not fit after printing.
**Mitigation:** Add 0.5mm clearance around M3 holes (3.5mm hole for M3 bolt). This gives installation wiggle room without compromising mechanical stability.

### 10. Heatsink Omission on Raspberry Pi
**What happens:** The Pi 4 CPU under sustained YOLOv8 inference reaches ~70–80°C without a heatsink. At 80°C the SoC thermally throttles to 1.5GHz (from 1.8GHz). Throttled Pi = lower inference FPS = slower following response. At 85°C it throttles further to 1.0GHz.
**Mitigation:** Add a self-adhesive aluminum heatsink to the Pi 4 SoC (BCM2711 — the large chip). Cost: $1–2. Reduces operating temperature by 15–20°C under load. Optional: 30mm × 5V blower fan on the chassis directed at the Pi if temperatures are still marginal after heatsink.

---

## PCB Design Checklist

Run through this before exporting Gerbers. Every unchecked item is a potential board respin.

**Schematic:**
- [ ] ESP32-WROOM-32 symbol with correct pin mapping
- [ ] 100nF + 10µF decoupling on ESP32 VCC
- [ ] TB6612FNG × 2 — all control pins connected to ESP32
- [ ] STBY pin on both TB6612FNG tied to 3.3V
- [ ] AMS1117-3.3 with 10µF + 100nF on input and output
- [ ] MP1584EN with feedback resistors set for 5.0V output and 100µF output cap
- [ ] Reverse polarity MOSFET on VBAT input
- [ ] 5A polyfuse on VBAT line
- [ ] 1kΩ/2kΩ voltage dividers on all 3 HC-SR04 Echo lines
- [ ] 4.7kΩ I2C pull-ups on SDA and SCL to 3.3V
- [ ] Battery voltage sense divider (10kΩ + 3.3kΩ) to ESP32 ADC pin
- [ ] Boot button (GPIO0 to GND via tactile button)
- [ ] Reset button (EN to GND via tactile button with 10kΩ pull-up)
- [ ] Status LED with ~100Ω current limiting resistor from 3.3V
- [ ] ERC runs clean — zero errors

**Layout:**
- [ ] TB6612FNG chips placed adjacent to motor output connectors
- [ ] Motor output traces ≥ 1.2mm
- [ ] VBAT input trace ≥ 2.5mm
- [ ] 5V RPi trace ≥ 1.5mm
- [ ] Encoder traces routed away from motor current traces (opposite PCB side if possible)
- [ ] 100nF caps within 2mm of each IC power pin
- [ ] Ground plane poured on both layers, stitching vias every 10mm
- [ ] Connector labels in silkscreen (polarity marked on motor connectors)
- [ ] Board outline matches dimensions communicated to mechanical team
- [ ] DRC runs clean — zero errors
- [ ] Gerbers visually verified in viewer before upload to JLCPCB

---

## Timeline

| Week | Target |
|---|---|
| 1–2 | Schematic 80% — ESP32, motor drivers, power block, all connectors |
| 2–3 | Schematic complete, ERC clean. Layout started. |
| 3 | Layout complete, DRC clean. Gerbers to JLCPCB. Order 5 boards. |
| 4–5 | PCB arrives (~7–10 days). Solder. Power-on test. |
| 5 | Motor spin confirmed. Encoder pulses verified on oscilloscope or logic analyzer. |
