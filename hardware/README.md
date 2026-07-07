# Hardware — Philo PCB

**Owner: [@dish-glitch](https://github.com/dish-glitch)**

---

## Overview
The PCB was designed as the main interface for philo, it distributes the power and connects the raspberry Pi and ESP32. It drives the motors, and provides connectors for the sensors used throughout the robot. 

**Current phase: PCB fabrication.** Schematic complete (0 ERC errors), PCB layout complete (DRC clean, Gerbers exported). Kicad files: [`hardware/kicad/`](kicad/).

**For the actual build, start here:**
- **[BOM.md](BOM.md)** — complete parts list 
- **[SCHEMATIC.md](SCHEMATIC.md)** — completed, for reference


---

## Component Specifications (Verified from Datasheets)

The components below are the parts selected for the current revision of the PCB. Main componets are here incase of debugging. 

| Component | Key Specs | Source | Weight |
|---|---|---|---|
| ESP32-WROOM-32 | 240MHz dual-core, 520KB SRAM, 3.3V, 18x25.5x3.1mm | [Espressif datasheet](https://www.espressif.com/sites/default/files/documentation/esp32-wroom-32d_esp32-wroom-32u_datasheet_en.pdf) | 2.5g |
| TB6612FNG x2 | 1.2A continuous / 3.2A peak per channel, 4.5-13.5V motor, 2.7-5.5V logic, 100kHz PWM max | [Toshiba datasheet](https://cdn-shop.adafruit.com/datasheets/TB6612FNG_datasheet_en_20121101.pdf) | 1.5g each |
| MPU-6050 (GY-521 module) | 3-axis accel +/-16g, 3-axis gyro +/-2000 deg/s, I2C, 3.3-5V, 21x15mm | [InvenSense datasheet](https://product.tdk.com/en/search/sensor/mortion-inertial/imu/info?part_no=MPU-6050) | 13g |
| HC-SR04 x3 | 5V, <15mA, 2-400cm range, 40kHz, 45x20x15mm | [Sparkfun datasheet](https://cdn.sparkfun.com/datasheets/Sensors/Proximity/HCSR04.pdf) | 10g each |
| AMS1117-3.3 LDO | 3.3V out, 800mA max, SOT-223 package | AMS datasheet | <1g |
| Pololu D24V50F5 Buck (5V/5A) | 4.5-38V in, 5.0V fixed out, 5A continuous near capacity with Pi 5 (up to 4.5A peak) | [Pololu D24V50F5](https://www.pololu.com/product/2851) | ~5g |
| JGA25-370 x4 | 6V, 200 RPM, 1.8-2.2 kg*cm stall torque, 4mm D-shaft, 25mm body diameter | Product page | 90g each |
| 2S LiPo 2200mAh | 7.4V nominal, 8.4V full, 6.4V cutoff, 97-130g depending on model | Gens Ace product page | ~110g |
| XT30 Connector | 30A continuous, gold plated, compact | XT30 spec | 3g per pair |

---

## PCB Block Architecture

### Block 1 — Microcontroller: ESP32-WROOM-32

the ESP32 is responsible for the robot's real time control. it hands the motor control, reads the ultrasonic sensors and IMU. it also communicates with the PI over UART and updates the oled screens. 

**Decoupling:** we placed one 100nF ceramic cap and one 10uF electrolytic cap between VCC and GND, as close to the module as possible. These capacotors help keep the esp32 stable during current changes. 

**Brownout detection:** we enabled the ESP32 hardware brownout detector in firmware and set brownout threshold to 2.7V via esp_reset_reason and brownout config. This allows the esp32 to reset cleanly rather than staying in an undefined state. 

**Programming:** 6-pin header (TX, RX, EN, GPIO0, 3.3V, GND). CP2102 plugs in here!

Boot button: tactile button from GPIO0 to GND needed for entering flash mode. 

### Block 2 — Motor Drivers: TB6612FNG x2 

Two TB6612FNG ICs. Each drives two motors. Total: 4 motors driven.

**Why we chose the TB6612FNG:** Early in the project we were going to pick the L298N becuase its common and used in many starting projects, but after looking at the voltage drop and power loss we decided to use the TB6612FNG. Because it uses MOSFET instead of bipolar transistors it wastes less power while aslo delivering more of the battery voltage to the motors.  

**Wiring each TB6612FNG:**
- PWMA, AIN1, AIN2 from ESP32 GPIO
- AO1, AO2 to motor A terminals
- PWMB, BIN1, BIN2, BO1, BO2 for motor B
- STBY tied HIGH (3.3V) permanently — LOW disables all outputs
- VM to VBAT (motor voltage, up to 13.5V)
- VCC to 3.3V logic

**TB6612FNG thermal note with JGA25-370 motors:** The JGA25-370 at 7.4V running at 40% load draws ~0.5A per motor. However, at brief stall (before the firmware watchdog triggers at 500ms), stall current at 7.4V can reach 1.5-2.2A per motor, exceeding the 1.2A continuous rating but within the 3.2A peak. The IC handles this for brief moments. Ensure the TB6612FNG KiCad footprint has its exposed thermal pad connected to a GND copper pour on the PCB. This provides a thermal path from the die to the board. If either IC is hot to the touch after Test 2 (repeated reversals), increase the copper pour area under the chip.

**Back-EMF on motor reversal:** we also found that the when motor is commanded to reverse direction mid-spin, it briefly acts as a generator. This creates a voltage spike back into the VM pin. The TB6612FNG has internal flyback diodes on all output transistors that clamp this spike to VM. The 470uF bulk cap at each TB6612FNG VM pin (see Section: Ground Architecture and Decoupling) further absorbs these transients.

**Placement:** Both TB6612FNG ICs on the same side as motor output connectors. Motor output traces (AO1, AO2, BO1, BO2) minimum 1.2mm. Short and direct to connectors.

### Block 3 — Power Architecture

The main power system. This is one of the most important parts of the PCB due to its important purpose of reliablly supplying power to the raspberry pi, esp32, motors, and the sensors.  

**Input: 2S LiPo via XT30**

Voltage range: 6.4V (discharged) to 8.4V (fully charged). 7A-rated polyfuse on input line (e.g., Littelfuse RGEF700) — trips at sustained overcurrent, resets after cooldown. P-channel MOSFET for reverse polarity protection.

**Why 7A, not 5A:** A polyfuse has nonzero resistance that increases as it heats up. A 5A-rated polyfuse at 3A normal load runs warm and its resistance rises to 0.1-0.5 ohm, causing 0.3-1.5V sag on VBAT before it ever trips. A 7A-rated part runs cold at our 3A normal current (resistance stays in milliohm range), so there is no sag during operation. Protection still works — fault currents above 7A trip it cleanly.

**3.3V Rail: AMS1117-3.3 LDO**

Powers: ESP32, MPU-6050, TB6612FNG logic, sensor pull-ups, LEDs.

**Input: 5V rail (from D24V50F5 output) — NOT from VBAT directly.**

Reason: At 8.4V VBAT input, the AMS1117 dissipates (8.4 - 3.3) x 0.27A = 1.38W. In SOT-223, that is a junction temperature of ~135C at 25C ambient, which is at or above the 125C maximum rating. Fed from 5V instead, dissipation is (5.0 - 3.3) x 0.27A = 0.46W, estimated junction temperature ~62C. Safe.

One reviewer concern was that cascading from 5V "couples Pi rail noise into ESP32 rail." This is overstated: the AMS1117 is a linear regulator with ~65dB PSRR at low frequencies, meaning 5V ripple is attenuated 1800x at the 3.3V output. If the 5V rail sags badly enough to matter for ESP32, the Pi is already browned out — that is a separate problem addressed by the 1000uF cap.

Caps: 10uF electrolytic + 100nF ceramic on both input and output pins. Required for AMS1117 stability — without them the output oscillates at high frequency.

Max load: ~270mA. Rated 800mA. Adequate headroom.

**5V Rail: Pololu D24V50F5 Buck Converter (5V, 5A)**

> Revision history: MP2307 (1.2A) was the original part — far too small, would have burned out. Then MP1584EN (3A) — adequate but tight: Pi at 2.5-3.0A + AMS1117 cascade 0.27A = ~2.77A on a 3A part, marginal once derated for an enclosed chassis. Final part is the **D24V50F5 (5A)** for real headroom on the most demo-critical rail. Pi brownout is the #1 demo-killer, so this rail gets margin.

Input: VBAT (6.4-8.4V)
Output: 5.0V (fixed — no trimpot to set, unlike the MP1584EN module)
EN pin: tie to VIN or leave floating (internal pull-up enables by default). PG (power-good) pin optional.

**Total 5V load: Pi 3.5A typical (4.5A peak) + AMS1117 cascade 0.27A = ~3.77A typical.** D24V50F5 rated 5A. At peak Pi load the margin is only ~0.5A — monitor for throttling during full inference. Active cooler on the Pi 5 mitigates thermal throttling and reduces average power draw.

**Output capacitors — updated from earlier revision:**
- 1000uF electrolytic (upgraded from 100uF based on Pi inrush spike analysis)
- 100nF ceramic in parallel

Why 1000uF: When the Pi 5 CPU ramps from idle to full inference load, it draws a current spike faster than the converter's control loop can respond (~microseconds). The 1000uF cap supplies that spike locally while the converter catches up. At a 3A spike for 1ms, 1000uF holds the voltage within 3mV of setpoint. 100uF would sag by 30mV — enough to trigger the Pi's undervoltage warning. Keep this cap even with the 5A buck 

5V connects to Pi via 2-pin header to GPIO pin 2 (5V) and pin 6 (GND). 

**Power sequencing note:** ESP32 boots in ~300ms. Pi takes 20-30 seconds. Firmware default state: all motors stopped, watchdog running. Motors do not move until first valid CMD packet received from Pi.

### Block 4 — Ground Architecture and Decoupling

While reviewing the power system, I realized the PCB layout mattered just as much as the schematic. Most of the components were already connected correctly, but poor grounding can still cause random resets and communication issues

**The problem:** Motor switching at 20kHz creates high-frequency current spikes through the ground path. If motor return current and ESP32 ground share any trace segment, the voltage difference across that trace's resistance injects noise into the ESP32's ground reference. This causes:
- Ghost resets when motors start or reverse
- UART data corruption on sharp direction changes
- Ultrasonic echo timing errors during motor switching
- IMU readings spiking during acceleration

**The fix — ground plane:**

Use the bottom copper layer as a solid ground plane. In KiCad: copper fill on back copper layer, set to GND net, pour.

This gives all ground connections (motor return, ESP32 GND, sensor GND, LDO GND) a low-impedance path back to the battery negative terminal.

**Single star point:** The battery negative (GND from XT30) should be the first ground connection on the board. Everything else connects to the plane, which connects to this point. 

**Bulk capacitors at each TB6612FNG:**

we have one 470uF electrolytic capacitor directly at each TB6612FNG VM pin (motor supply voltage). These absorb the current spike when a motor starts from rest or reverses direction. Without them, the spike travels back to the battery and couples into the 3.3V and 5V rails through shared impedance.

Component count: 2x 470uF (one per TB6612FNG IC), placed within 3mm of the VM pin.

**Decoupling cap placement rules:**
- 100nF ceramic caps: within 2mm of every IC power pin
- 470uF motor bulk caps: within 3mm of TB6612FNG VM pins
- 1000uF Pi rail cap: within 5mm of D24V50F5 output
- All caps connect to ground plane via short vias, not via trace segments

### Block 5 — IMU: MPU-6050

Connects via I2C (SDA/SCL). Default address 0x68. Document I2C address map.

Mount flat and horizontal on the PCB. 4.7k pull-ups on SDA and SCL to 3.3V. ESP32 internal pull-ups are too weak for reliable I2C.

Implement I2C bus recovery routine in firmware (9 SCL clocks with SDA held high) to clear SDA-stuck-low lockups from Pi resets mid-transaction.

**I2C address map (three devices on one bus):**

| Device | I2C Address | Function |
|---|---|---|
| MPU-6050 | 0x68 | Tilt / tip-over detection |
| OLED left eye | 0x3C | Companion display |
| OLED right eye | 0x3D | Companion display |

The bus has 4.7k pull-ups which should be adequate for all three devices on a short bus (under ~15cm total trace length). Keep the I2C traces short and route them away from motor traces.

### Block 5b — Companion Display: 2x OLED Eyes

Philo is a companion robot, so it has an animated face. Two SSD1306 (or SH1106) OLED modules, 128x64, I2C, mounted at the top of the camera mast as eyes.

- Addresses 0x3C and 0x3D, set by the address-select pad on each module (most SSD1306 breakouts have a solderable jumper to pick 0x3C or 0x3D)
- Power: 3.3V, ~20mA each (~40mA total) — negligible
- Driven by the ESP32 on core 1 (see firmware README — full refresh is ~23ms per display, so it must NOT run in the control loop)
- Connect via the existing I2C bus — no new GPIO pins needed

**Mechanical:** hummos430 needs two OLED-sized cutouts in the front of the camera mast head, positioned as eyes. OLED active area is ~22mm x 11mm; module PCB is ~27mm x 27mm. Mount with M2 screws or a friction pocket.

### Block 5c — Audio: handled by the Raspberry Pi, NOT the ESP32

Sound (personality chirps, "found you" / "waiting" clips) is driven by the Raspberry Pi, not the ESP32.

**Why not the ESP32:** the ESP32 is nearly out of usable GPIO. The only free pins (0, 12, 15) are all boot-strapping pins, which makes driving an I2S speaker amp off them fragile. The Pi has free USB ports, a filesystem for WAV clips, and trivial audio playback (aplay). It also already knows the high-level state (found/lost/gesture) that should trigger sounds.

**Hardware (Pi side, not on this PCB):**
- A cheap USB audio adapter + small amplified speaker, OR a small self-powered USB speaker
- Powered from the Pi USB port (small speaker) or tapped from the 5V rail (budget ~600mA peak, intermittent — see power budget)

### Block 6 — Ultrasonic Sensors: HC-SR04 x3

Mount on chassis, connect via 4-pin headers. Echo pins MUST go through voltage dividers before reaching ESP32 GPIO — HC-SR04 echo is 5V, ESP32 max input is 3.6V.

```
HC-SR04 Echo (5V) ---- 1k ----+---- ESP32 GPIO (3.33V)
                               |
                              2k
                               |
                              GND
```

Here is the calculation for this :D
Vout = Vin × R2/(R1+R2)

= 5V(ECHO pin) × 2k/(1k+2k)

= 3.33V


**HC-SR04 crosstalk** if we fire all of the sensors at once for example: If sensor A fires while sensor B is listening, sensor B will hear sensor A's pulse as a reflection and report a obstacle a few centimeters away even though there is no obstacle causing the esp32 stop. we made the firmware fire sensors sequentially with a 60ms delay between each that way it it eliminates the risk of ghost stopping. See firmware README for timing diagram.

**Cable routing for sensor wires:** we have to keep the HC-SR04 cables short (under 20cm). Motor PWM switching creates noise on the ultrasonic signals when the wires were routed together so we plan to make srue the sensor wires are away from the motor wires to reduce false reading. 

### Block 7 — Brownout Visibility

indecators to help find out why something has stopped working. 

**Status LED behavior (firmware controls this, but hardware must support it):**
- Solid ON: normal operation
- Fast blink (5Hz): low battery warning (below 6.4V)
- Double flash pattern: ESP32 reset from brownout
- Off: power loss

**Battery sense circuit (add to schematic):**

A resistor divider from VBAT to an ESP32 ADC pin lets the firmware monitor battery voltage:

```
VBAT ---- 10k ----+---- ESP32 ADC (GPIO36, input-only pin)
                  |
                 3.3k
                  |
                 GND
```

At 8.4V battery: ADC reads 8.4 x (3.3/(10+3.3)) = 2.08V
At 6.4V battery: ADC reads 6.4 x (3.3/(10+3.3)) = 1.58V

Both within ESP32 ADC range (0-3.3V). When the ADC reading drops to the value corresponding to about 6.4V battery voltage, the firmware warns the user and can disable high-current operations.

---

## Power budget 

| Component | Voltage Rail | Typical Current | Peak (transient) | Source | 
|---|---|---|---|---|
| Raspberry Pi 5 w/ active cooler (inference load) | 5V | 3.5A | 4.5A | Raspberry Pi 5 requires more power than our original Pi 4 plan, especially during AI inference. The 5V convertor is reaching its limit so we plan to monitor it. -- this is a estimate.  |
| 4x JGA25-370 motors (running load at 40% torque) | VBAT | 2.0A total | 8.8A (The polyfuse is only a backup safety layer. It is not expected to handle normal motor stalls because the firmware watchdog should disable the motors first.) | Motor spec + stall analysis |
| 4x JGA25-370 motor reversal transient | VBAT | — | ~4.4A for <50ms | Back-EMF analysis (2 motors reverse at once) |
| ESP32 (active, WiFi off) | 3.3V | 240mA | 340mA | Espressif datasheet |
| 3x HC-SR04 | 5V | 45mA | same | HC-SR04 datasheet |
| MPU-6050 | 3.3V | 3.9mA | same | InvenSense datasheet |
| 2x OLED eyes | 3.3V | 40mA | same | SSD1306 spec |
| Speaker (Pi-side, if on 5V rail) | 5V | 50mA idle | ~600mA peak (intermittent) | MAX98357A / USB speaker spec |
| Misc (LEDs, pull-ups) | 3.3V | 20mA | same | Estimated |
| **Total normal operation** | | ~4.8A | | |
| **Motor reversal transient** | | | ~4.4A for <50ms | Absorbed by 470uF caps |
| **All motors stalled** | | ~8.8A sustained | | Above 7A hold — polyfuse trips slowly; firmware watchdog (500ms) is primary stall protection |

**Note on speaker peak:** the ~600mA speaker peak is brief (during a sound clip) and stacks on the 5V rail. With the 5A D24V50F5, total worst case (~3.77A typical + 0.6A = ~4.4A) still fits under the 5A rating.

**Battery runtime:** we estimated that the battery can run for about 27 minutes.  2200mAh x 7.4V = 16.3Wh. At ~35W normal draw: ~27 minutes. 

**Polyfuse behavior at all-motors-stall:** 4x JGA25-370 at 7.4V stall current ~1.5-2.2A each = 6-8.8A total. The RGEF700 (7A hold, ~14A trip current) at 8.8A is only 1.26× hold — it will trip but slowly (minutes, not seconds). This is intentional: the 7A part was chosen over 5A specifically to avoid pre-trip resistance sag at normal 3A load. At stall, the firmware 500ms watchdog stops the motors long before the polyfuse acts. The polyfuse is the last-resort backstop for a firmware crash, not the primary stall protection.

**Motor reversal transient:** During reversal, motor acts as generator. Two motors reversing simultaneously gives ~4.4A transient for <50ms. Two 470uF caps at TB6612FNG VM pins (one per IC) absorb this: 0.47 coulombs capacity, transient requires 0.22 coulombs. This calculation was done as a rough check to verify the capacitors were large enough to reduce voltage dips during motor direction changes.

---

## Trace Width Requirements

| Net | Max Current | Minimum Width | Notes |
|---|---|---|---|
| VBAT input | 9A peak (stall) | 2.5mm | Size for fault condition |
| Motor outputs (per TB6612FNG channel) | 1.2A cont, 3.2A peak | 1.2mm | Short, direct to connector |
| 5V RPi rail | 3A | 1.5mm | Full Pi inference current |
| 3.3V logic rail | 500mA | 0.8mm | Include margin |
| Signal traces (UART, I2C, GPIO) | <50mA | 0.25mm | KiCad default |

---

## Electrical Stress Test Protocol

Before trusting any demo, run these tests in order. If the robot passes all four, the power system is solid.

**Test 1 — Full throttle forward for 30 seconds**
Command `CMD 255 255 0`. Measure battery voltage at start and end. Should not sag more than 0.3V. Feel the TB6612FNG ICs and the D24V50F5 module — should be warm but not hot. If either TB6612FNG is too hot to touch, increase copper pour area under it.

**Test 2 — Repeated hard reversal**
Alternate `CMD 255 255 0` and `CMD -255 -255 0` every 2 seconds, 20 times. Watch for ESP32 resets or Pi undervoltage icon. This is the worst case for back-EMF transients and TB6612FNG thermal stress.

**Test 3 — Stall one wheel**
Block one wheel by hand for 3 seconds. Robot should not reset. Polyfuse should not trip (single motor stall is ~1.8A, well under 7A hold current). Release and confirm motors spin up correctly.

**Test 4 — Full system under inference**
Run full YOLO stack on Pi while robot follows. Watch Pi temperature (vcgencmd measure_temp). Target under 70C with heatsink. If throttling occurs (check with vcgencmd get_throttled), add a 30mm fan. -- (UPDATE: this test wasd one Pi's average temp was 61C)

---
## Initial Test Results

| Test | Result |
|---|---|
| Full throttle | PENDING, voltage drop measured at ___V |
| Hard reversal | PENDING, no ESP32 resets |
| Single motor stall | PENDING |
| YOLO inference | Temp averaged with 61C with active cooler running YOLO and media. |

Measurement method for yolo interface: 
- vcgencmd measure_temp
- YOLOv8 pose inference active
- media playback active
- arduino with OLED, MPU6050, and ultrasonic sensor active. 

## Failure Mode Analysis(FMEA)

### 1. Motor Burn from Sustained Stall
**Cause:** Wheel jammed, motor at stall current (~1.5-2.2A at 7.4V).
**Mitigation:** The ESP32 watchdog stops the motors if commands stop arriving from the Pi for 500ms. This protects against cases where the Pi crashes or communication is interrupted.

### 2. RPi Brownout from 5V Sag
**Cause:** Pi current spike faster than buck converter response.
**Mitigation:** D24V50F5 (5A rated, ~0.5A headroom at Pi 5 peak) + 1000uF output cap. The output capacitor helps handle short current spikes before the buck converter can react


### 3. Ghost Resets from Ground Bounce
**Cause:** Motor switching current through shared ground traces injects noise into ESP32 GND reference.
**Mitigation:** Full ground plane on bottom PCB layer, stitching vias. 470uF bulk caps at each TB6612FNG VM pin. Single star ground entry point at XT30 connector. This was added because motor currents are much larger than sensor currents. 

### 4. ESP32 GPIO Damage from HC-SR04 5V Echo
**Cause:** HC-SR04 echo output is 5V. ESP32 GPIO max input 3.6V.
**Mitigation:** 1k/2k voltage divider on every echo line. 

### 5. Battery Over-Discharge
**Cause:** 2S LiPo below 3.0V/cell (6.0V total) causes permanent cell damage and swelling.
**Mitigation:** Battery voltage sense circuit (10k/3.3k divider to ADC). Firmware cuts motors and flashes LED at 6.4V (3.2V/cell). Also saves the end of a test run. THe LED warning gives time to land gracefully :P.

### 6. Motor Wiring Polarity Error
**Cause:** Motor connector plugged backward. One side drives in reverse. Robot spins in circles 👎.
**Mitigation:** Polarity marked on PCB silkscreen. Test each motor independently with `CMD 100 100 0` before full assembly.

### 7. I2C Bus Lockup
**Cause:** if coms is interrupted while the MPU-6050 is sending data it can leave SDA stuck low and cause other I2C devices from working. 
**Mitigation:** I2C bus recovery routine in firmware init (9 SCL clocks while SDA high). Runs before main loop.

### 8. UART Garbage on Pi Boot
**Cause:** Pi kernel uses GPIO14 (UART TX) for boot messages. Arrives at ESP32 as garbage bytes.
**Mitigation:** Firmware discards any line that does not start with the `CMD` prefix. Also: disable serial console in raspi-config on Pi setup (required step, see vision README).

### 9. PWM Noise on Encoder/Sensor Lines
**Cause:** 20kHz PWM couples capacitively into parallel sensor wires. Adds noise to encoder counts and HC-SR04 echo timing.
**Mitigation:** 100nF cap on each encoder signal line to GND at ESP32 pin. Route sensor cables away from motor cables. Keep ultrasonic cables under 20cm.

### 10. Pi Thermal Throttling Under Inference
**Cause:** Pi 5 SoC reaches 85C without cooling. Throttles from 2.4GHz down. Drops YOLO FPS. Slows following response.
**Mitigation:** Active cooler already installed on Pi 5 (from friend). 

### 11. HC-SR04 Simultaneous Firing (Crosstalk)
**Cause:** All three sensors fired at the same time. Sensor B hears Sensor A's pulse as a phantom reflection at ~0 cm. ESP32 registers a phantom wall directly in front and triggers the escape sequence every cycle.
**Mitigation:** Sequential sensor firing with 60ms minimum delay between each sensor. 

### 12. TB6612FNG Overheating with JGA25-370
**Cause:** JGA25-370 stall current (1.5-2.2A at 7.4V) exceeds TB6612FNG continuous rating (1.2A). Sustained near-stall conditions cause IC to overheat.
**Mitigation:** Exposed thermal pad on TB6612FNG QFN package must be connected to GND copper pour in KiCad footprint. Firmware soft-start (PWM ramp over 200ms) Run Test 2 and confirm ICs are warm and NOT toasty.

### 13. Control Loop Oscillation
**Cause:** 200ms vision-to-motor latency combined with aggressive turn commands. Robot overshoots center, camera now sees person on opposite side, sends opposite turn command, overshoots again. Robot sways left-right following person.
**Mitigation:** Exponential moving average on offset value in Pi code (0.7 x previous + 0.3 x new). Dead zone ±0.15 already helps but is not sufficient alone. See vision README for implementation.

### 14. Pi/ESP32 State Desync
**Cause:** Pi sends STOP (enters WAITING state). ESP32 is mid-execution of last forward command. 100-300ms of "ghost motion" occurs — robot moves when it should be stopped.
**Mitigation:** Pi flushes serial write buffer before entering WAITING state. ESP32 watchdog at 500ms handles the case where Pi goes silent entirely.

### 15. Ultrasonic Sensor Dropout Cascade
**Cause:** HC-SR04 returns no echo (object too close, sensor disconnected, electrical noise). Firmware interprets timeout as either 0cm (triggers escape loop constantly) or max range (treats path as clear when it is not).
**Mitigation:** Readings outside 2-400cm range flagged as INVALID. Firmware holds last known valid reading for up to 3 cycles (540ms) before reverting to safe default (treat as obstacle at 20cm). See firmware README.

---
## Future Testing. 
Planned:
- [ ] Complete motor stress testing after PCB assembly
- [ ] Measure actual battery runtime
- [ ] Validate thermal performance under continuous following
- [ ] Tune obstacle avoidance parameters
---

## PCB Design Checklist

**Schematic (COMPLETE — 0 ERC errors, all footprints assigned):**
- [x] ESP32-WROOM-32 with 100nF + 10uF decoupling on VCC
- [x] TB6612FNG x2 (U6, U7) — all control pins connected, STBY tied HIGH
- [x] 470uF bulk cap at each TB6612FNG VM pin (2 caps total)
- [x] AMS1117-3.3 (U5) with 10uF + 100nF on input and output
- [x] D24V50F5 (5V/5A) module placed via J8; VIN←VBAT, VOUT→+5V; 1000uF + 100nF output caps
- [x] Reverse polarity MOSFET Q2 (AOD4185 P-channel) on VBAT input
- [x] 7A-rated polyfuse F2 (Littelfuse RGEF700) on VBAT line
- [x] AMS1117-3.3 input connected to 5V rail (D24V50F5 output) — NOT to VBAT
- [x] 1k/2k voltage dividers on all 3 HC-SR04 Echo lines (6 resistors)
- [x] 4.7k I2C pull-ups on SDA and SCL to 3.3V
- [x] Battery voltage sense divider (10k + 3.3k) to ESP32 ADC pin (GPIO36)
- [x] Boot button SW2 (GPIO0 to GND)
- [x] Reset button SW1 (EN to GND with 10k pull-up)
- [x] Status LED D1 with 100Ω current limiting resistor R11
- [x] Test points TP1–TP6 on VBAT, +5V, +3V3, GND, ESP_TX, ESP_RX
- [x] Encoder connectors J13 (ENC_L) and J14 (ENC_R)
- [x] ERC clean — 0 errors

**Layout (COMPLETE — DRC clean, Gerbers exported):**
- [X] Ground plane poured on bottom copper layer, net = GND
- [X] Stitching vias every 10mm across board area
- [X] Single star ground entry at XT30 negative terminal
- [X] TB6612FNG exposed thermal pad connected to GND copper pour (NOT floating) — verify in KiCad footprint properties
- [X] D24V50F5 module footprint matches Pololu pin layout; GND pads to ground pour
- [X] 470uF bulk caps within 3mm of each TB6612FNG VM pin
- [X] 1000uF cap within 5mm of D24V50F5 output pin
- [X] All 100nF decoupling caps within 2mm of IC power pins
- [X] TB6612FNG placed adjacent to motor output connectors
- [X] Motor output traces >= 1.2mm
- [X] VBAT traces >= 2.5mm
- [X] 5V RPi traces >= 1.5mm
- [X] Encoder traces routed away from motor traces (opposite PCB side preferred)
- [X] Polarity marked on motor connectors in silkscreen
- [X] Board outline matches dimensions given to mechanical team
- [x] DRC clean — zero errors
- [x] Gerbers exported and verified in viewer

---

## Timeline

| Week | Target | Status |
|---|---|---|
| 1-2 | Schematic complete — all blocks, all decoupling, all connectors, ERC clean | **DONE** |
| 2-3 | PCB layout with ground plane, DRC clean, Gerbers exported | **DONE** |
| 3 | Gerbers to JLCPCB, 5 boards ordered | Next |
| 4-5 | PCB arrives, solder, power-on test | — |
| 5 | Run electrical stress test protocol before connecting Pi | — |

