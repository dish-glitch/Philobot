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
| ESP32-WROOM-32 | 240MHz dual-core, 520KB SRAM, 3.3V, 18x25.5x3.1mm | [Espressif datasheet](https://www.espressif.com/sites/default/files/documentation/esp32-wroom-32d_esp32-wroom-32u_datasheet_en.pdf) | 2.5g |
| TB6612FNG x2 | 1.2A continuous / 3.2A peak per channel, 4.5-13.5V motor, 2.7-5.5V logic, 100kHz PWM max | [Toshiba datasheet](https://cdn-shop.adafruit.com/datasheets/TB6612FNG_datasheet_en_20121101.pdf) | 1.5g each |
| MPU-6050 (GY-521 module) | 3-axis accel +/-16g, 3-axis gyro +/-2000 deg/s, I2C, 3.3-5V, 21x15mm | [InvenSense datasheet](https://product.tdk.com/en/search/sensor/mortion-inertial/imu/info?part_no=MPU-6050) | 13g |
| HC-SR04 x3 | 5V, <15mA, 2-400cm range, 40kHz, 45x20x15mm | [Sparkfun datasheet](https://cdn.sparkfun.com/datasheets/Sensors/Proximity/HCSR04.pdf) | 10g each |
| AMS1117-3.3 LDO | 3.3V out, 800mA max, SOT-223 package | AMS datasheet | <1g |
| MP1584EN Buck Converter | 4.5-28V in, adjustable out, 3A continuous, 1.5MHz switching, SOP-8 | MPS datasheet | <1g |
| JGA25-370 x4 | 6V, 200 RPM, 1.8-2.2 kg*cm stall torque, 4mm D-shaft, 25mm body diameter | Product page | 90g each |
| 2S LiPo 2200mAh | 7.4V nominal, 8.4V full, 6.4V cutoff, 97-130g depending on model | Gens Ace product page | ~110g |
| XT30 Connector | 30A continuous, gold plated, compact | XT30 spec | 3g per pair |

---

## PCB Block Architecture

### Block 1 — Microcontroller: ESP32-WROOM-32

The ESP32 is the only firmware-running chip on this board. It handles everything: motor PWM, PID loops, serial comms with the Pi, ultrasonic polling, and IMU reading.

The module solders to castellated pads on the PCB edge. Use the official KiCad footprint from the Espressif KiCad library.

**Decoupling:** Place one 100nF ceramic cap and one 10uF electrolytic cap between VCC and GND, as close to the module as possible. Without these, the module can brownout during switching transients.

**Brownout detection:** Enable the ESP32 hardware brownout detector in firmware (set brownout threshold to 2.7V via esp_reset_reason and brownout config). When triggered, the ESP32 resets cleanly rather than hanging in an undefined state. Wire the status LED to flash a distinct pattern on brownout reset so you can see if it is happening during testing.

**Programming:** 6-pin header (TX, RX, EN, GPIO0, 3.3V, GND). CP2102 plugs in here. Not soldered to board permanently.

Boot button: tactile button from GPIO0 to GND. Required for entering flash mode.

### Block 2 — Motor Drivers: TB6612FNG x2

Two TB6612FNG ICs. Each drives two motors. Total: 4 motors driven.

**Why not L298N:** L298N bipolar transistors drop ~2V per switch at 1A, wasting 24% of available motor voltage as heat and requiring a heatsink. TB6612FNG uses MOSFETs with near-zero drop. No heatsink. Full battery voltage to motors.

**Wiring each TB6612FNG:**
- PWMA, AIN1, AIN2 from ESP32 GPIO
- AO1, AO2 to motor A terminals
- PWMB, BIN1, BIN2, BO1, BO2 for motor B
- STBY tied HIGH (3.3V) permanently — LOW disables all outputs
- VM to VBAT (motor voltage, up to 13.5V)
- VCC to 3.3V logic

**TB6612FNG thermal note with JGA25-370 motors:** The JGA25-370 at 7.4V running at 40% load draws ~0.5A per motor — well within the 1.2A continuous rating. However, at brief stall (before the firmware watchdog triggers at 500ms), stall current at 7.4V can reach 1.5-2.2A per motor, exceeding the 1.2A continuous rating but within the 3.2A peak. The IC handles this for brief moments. Ensure the TB6612FNG KiCad footprint has its exposed thermal pad (bottom of QFN package) connected to a GND copper pour on the PCB. This provides a thermal path from the die to the board. If either IC is hot to the touch after Test 2 (repeated reversals), increase the copper pour area under the chip.

**Back-EMF on motor reversal:** When a motor is commanded to reverse direction mid-spin, it briefly acts as a generator. This creates a voltage spike back into the VM pin. The TB6612FNG has internal flyback diodes on all output transistors that clamp this spike to VM. The 470uF bulk cap at each TB6612FNG VM pin (see Section: Ground Architecture and Decoupling) further absorbs these transients.

**Placement:** Both TB6612FNG ICs on the same side as motor output connectors. Motor output traces (AO1, AO2, BO1, BO2) minimum 1.2mm. Short and direct to connectors.

### Block 3 — Power Architecture

Power failures mid-demo are catastrophic. This block must be correct.

**Input: 2S LiPo via XT30**

Voltage range: 6.4V (discharged) to 8.4V (fully charged). 7A-rated polyfuse on input line (e.g., Littelfuse RGEF700) — trips at sustained overcurrent, resets after cooldown. P-channel MOSFET for reverse polarity protection.

**Why 7A, not 5A:** A polyfuse has nonzero resistance that increases as it heats up. A 5A-rated polyfuse at 3A normal load runs warm and its resistance rises to 0.1-0.5 ohm, causing 0.3-1.5V sag on VBAT before it ever trips. A 7A-rated part runs cold at our 3A normal current (resistance stays in milliohm range), so there is no sag during operation. Protection still works — fault currents above 7A trip it cleanly.

**3.3V Rail: AMS1117-3.3 LDO**

Powers: ESP32, MPU-6050, TB6612FNG logic, sensor pull-ups, LEDs.

**Input: 5V rail (from MP1584EN output) — NOT from VBAT directly.**

Reason: At 8.4V VBAT input, the AMS1117 dissipates (8.4 - 3.3) x 0.27A = 1.38W. In SOT-223, that is a junction temperature of ~135C at 25C ambient, which is at or above the 125C maximum rating. Fed from 5V instead, dissipation is (5.0 - 3.3) x 0.27A = 0.46W, junction temperature ~62C. Safe.

One reviewer concern was that cascading from 5V "couples Pi rail noise into ESP32 rail." This is overstated: the AMS1117 is a linear regulator with ~65dB PSRR at low frequencies, meaning 5V ripple is attenuated 1800x at the 3.3V output. If the 5V rail sags badly enough to matter for ESP32, the Pi is already browned out — that is a separate problem addressed by the 1000uF cap.

Caps: 10uF electrolytic + 100nF ceramic on both input and output pins. Required for AMS1117 stability — without them the output oscillates at high frequency.

Max load: ~270mA. Rated 800mA. Adequate headroom.

**5V Rail: MP1584EN Buck Converter**

> Correction from earlier revision: MP2307 was originally specified. MP2307 is rated 1.2A. Pi 4 draws up to 3A under inference. That would have burned out the converter mid-demo. MP1584EN is rated 3A. This is the correct part.

Input: VBAT (6.4-8.4V)
Output: 5.0V (set via feedback resistor divider, per section 7 of MP1584EN datasheet)

**Total 5V load: Pi 2.5A + AMS1117 cascade 0.27A = 2.77A.** MP1584EN rated 3A. Margin is 230mA — thin but adequate. In an enclosed chassis, derate to ~2.6A continuous. The margin holds under normal operation. If the chassis runs hot (hand-warm inside after 10 minutes), add a small copper pour or pad vent hole above the MP1584EN. Thermal pad on MP1584EN must be soldered to copper pour — same rule as TB6612FNG.

**Output capacitors — updated from earlier revision:**
- 1000uF electrolytic (upgraded from 100uF based on Pi 4 inrush spike analysis)
- 100nF ceramic in parallel

Why 1000uF: When the Pi 4 CPU ramps from idle to full inference load, it draws a current spike faster than the MP1584EN's control loop can respond (~microseconds). The 1000uF cap supplies that spike locally while the converter catches up. At a 3A spike for 1ms, 1000uF holds the voltage within 3mV of setpoint. 100uF would sag by 30mV — enough to trigger the Pi's undervoltage warning.

5V connects to Pi via 2-pin header to GPIO pin 2 (5V) and pin 6 (GND). Do not use the Pi USB-C port for power — cable run outside chassis is mechanically fragile.

**Power sequencing note:** ESP32 boots in ~300ms. Pi takes 20-30 seconds. Firmware default state: all motors stopped, watchdog running. Motors do not move until first valid CMD packet received from Pi.

### Block 4 — Ground Architecture and Decoupling

> This section was added after review of power transient failure modes. This is the most commonly skipped step in student PCB design and the most common source of unexplained random resets.

**The problem:** Motor switching at 20kHz creates high-frequency current spikes through the ground path. If motor return current and ESP32 ground share any trace segment, the voltage difference across that trace's resistance injects noise into the ESP32's ground reference. This causes:
- Ghost resets when motors start or reverse
- UART data corruption on sharp direction changes
- Ultrasonic echo timing errors during motor switching
- IMU readings spiking during acceleration

These failures are hard to debug because they look like software bugs.

**The fix — ground plane:**

Use the bottom copper layer as a solid ground plane. In KiCad: copper fill on back copper layer, set to GND net, pour. Stitch the planes with vias every 10mm across the board area.

This gives all ground connections (motor return, ESP32 GND, sensor GND, LDO GND) a low-impedance path back to the battery negative terminal. The impedance of a solid copper pour is milliohms. The impedance of a shared trace is ohms. The difference eliminates ground bounce.

**Single star point:** The battery negative (GND from XT30) should be the first ground connection on the board. Everything else connects to the plane, which connects to this point. Do not create a situation where motor return current must flow through the ESP32 ground pin to reach the battery.

**Bulk capacitors at each TB6612FNG:**

Place one 470uF electrolytic capacitor directly at each TB6612FNG VM pin (motor supply voltage). These absorb the current spike when a motor starts from rest or reverses direction. Without them, the spike travels back to the battery and couples into the 3.3V and 5V rails through shared impedance.

Component count: 2x 470uF (one per TB6612FNG IC), placed within 3mm of the VM pin.

**Decoupling cap placement rules:**
- 100nF ceramic caps: within 2mm of every IC power pin
- 470uF motor bulk caps: within 3mm of TB6612FNG VM pins
- 1000uF Pi rail cap: within 5mm of MP1584EN output
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

The 4.7k pull-ups are adequate for all three devices on a short bus (under ~15cm total trace length). Keep the I2C traces short and route them away from motor traces.

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

This keeps audio entirely off the custom PCB. The only PCB impact is the power budget allowance if the speaker is fed from the 5V rail. See vision README for the clip-playback software.

> Live ChatGPT voice conversation is explicitly a V2 / post-demo feature, not part of this build. It would require a microphone chain and would compete with YOLOv8 for the Pi's already-maxed CPU. For V1, audio is pre-recorded clips only.

### Block 6 — Ultrasonic Sensors: HC-SR04 x3

Mount on chassis, connect via 4-pin headers. Echo pins MUST go through voltage dividers before reaching ESP32 GPIO — HC-SR04 echo is 5V, ESP32 max input is 3.6V.

```
HC-SR04 Echo (5V) ---- 1k ----+---- ESP32 GPIO (3.33V)
                               |
                              2k
                               |
                              GND
```

Trigger lines (3.3V from ESP32) accepted by HC-SR04 as valid HIGH — no shifting needed.

**HC-SR04 crosstalk — CRITICAL:** Do not fire multiple sensors simultaneously. All three HC-SR04 units operate at 40kHz. If sensor A fires while sensor B is listening, sensor B will hear sensor A's pulse as a reflection and report a phantom obstacle a few centimeters away. This will trigger the escape sequence every cycle. Firmware must fire sensors sequentially with a 60ms delay between each. See firmware README for timing diagram.

**Cable routing for sensor wires:** Keep HC-SR04 cables short (under 20cm). Do not run them parallel to motor cables. EMI from motor PWM at 20kHz couples capacitively into parallel wires and adds noise to the echo timing. Use a twisted pair or at minimum route sensor cables along the opposite edge of the chassis from motor cables.

### Block 7 — Brownout Visibility

Add hardware indicators so failures are visible immediately during testing:

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

Both within ESP32 ADC range (0-3.3V). Firmware triggers low-battery behavior at 1.58V on the ADC.

---

## Real Power Budget (Verified Numbers)

| Component | Voltage Rail | Typical Current | Peak (transient) | Source |
|---|---|---|---|---|
| Raspberry Pi 4 (inference load) | 5V | 2.5A | 3.0A | Official Pi 4 power spec |
| 4x JGA25-370 motors (running load at 40% torque) | VBAT | 2.0A total | 8.8A (all motors stall at 7.4V — polyfuse trips) | Motor spec + stall analysis |
| 4x JGA25-370 motor reversal transient | VBAT | — | ~4.4A for <50ms | Back-EMF analysis (2 motors reverse at once) |
| ESP32 (active, WiFi off) | 3.3V | 240mA | 340mA | Espressif datasheet |
| 3x HC-SR04 | 5V | 45mA | same | HC-SR04 datasheet |
| MPU-6050 | 3.3V | 3.9mA | same | InvenSense datasheet |
| 2x OLED eyes | 3.3V | 40mA | same | SSD1306 spec |
| Speaker (Pi-side, if on 5V rail) | 5V | 50mA idle | ~600mA peak (intermittent) | MAX98357A / USB speaker spec |
| Misc (LEDs, pull-ups) | 3.3V | 20mA | same | Estimated |
| **Total normal operation** | | ~4.8A | | |
| **Motor reversal transient** | | | ~4.4A for <50ms | Absorbed by 470uF caps |
| **All motors stalled** | | ~8.8A sustained | | Polyfuse trips at 5A |

**Note on speaker peak:** the ~600mA speaker peak is brief (during a sound clip) and stacks on the 5V rail. The MP1584EN at 2.77A (Pi + AMS1117 cascade) plus a 600mA speaker peak approaches 3.4A — over the 3A rating for the duration of a loud clip. Keep clip volume moderate, or power the speaker from a USB power bank / the Pi USB port rather than the 5V buck rail. Do not play sound clips during peak motor + inference load if the rail is marginal.

**Battery runtime:** 2200mAh x 7.4V = 16.3Wh. At ~35W normal draw: ~27 minutes. Sufficient for demo. If runtime is marginal, upgrade to 3000mAh LiPo (same voltage, same connectors, adds ~35g).

**Why the polyfuse trips at all-motors-stall:** 4x JGA25-370 at 7.4V stall current ~1.5-2.2A each = 6-8.8A total. Polyfuse rated 5A trips within 1-2 seconds at 8A. This is the correct protective behavior.

**Motor reversal transient:** During reversal, motor acts as generator. Two motors reversing simultaneously gives ~4.4A transient for <50ms. Two 470uF caps at TB6612FNG VM pins (one per IC) absorb this: 0.47 coulombs capacity, transient requires 0.22 coulombs. Handled without sag.

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
Command CMD:F,255. Measure battery voltage at start and end. Should not sag more than 0.3V. Feel the TB6612FNG ICs and MP1584EN — should be warm but not hot. If either TB6612FNG is too hot to touch, increase copper pour area under it.

**Test 2 — Repeated hard reversal**
Alternate CMD:F,255 and CMD:B,255 every 2 seconds, 20 times. Watch for ESP32 resets or Pi undervoltage icon. None should occur. This is the worst case for back-EMF transients and TB6612FNG thermal stress.

**Test 3 — Stall one wheel**
Block one wheel by hand for 3 seconds. Robot should not reset. Polyfuse should not trip (single motor stall is ~1.8A, well under 5A). Release and confirm motors spin up correctly.

**Test 4 — Full system under inference**
Run full YOLO stack on Pi while robot follows. Watch Pi temperature (vcgencmd measure_temp). Target under 70C with heatsink. If throttling occurs (check with vcgencmd get_throttled), add a 30mm fan.

---

## Failure Mode Analysis

### 1. Motor Burn from Sustained Stall
**Cause:** Wheel jammed, motor at stall current (~1.5-2.2A at 7.4V) indefinitely.
**Mitigation:** 5A polyfuse trips on 3+ motor stall. Watchdog stops motors within 500ms of lost Pi comms. Obstacle escape sequence prevents sustained stall in normal operation.

### 2. RPi Brownout from 5V Sag
**Cause:** Pi current spike faster than buck converter response.
**Mitigation:** MP1584EN (3A rated) + 1000uF output cap. Cap supplies the spike while converter catches up.

### 3. Ghost Resets from Ground Bounce
**Cause:** Motor switching current through shared ground traces injects noise into ESP32 GND reference.
**Mitigation:** Full ground plane on bottom PCB layer, stitching vias. 470uF bulk caps at each TB6612FNG VM pin. Single star ground entry point at XT30 connector.

### 4. ESP32 GPIO Damage from HC-SR04 5V Echo
**Cause:** HC-SR04 echo output is 5V. ESP32 GPIO max input 3.6V.
**Mitigation:** 1k/2k voltage divider on every echo line. 6 resistors on the schematic.

### 5. Battery Over-Discharge
**Cause:** 2S LiPo below 3.0V/cell (6.0V total) causes permanent cell damage and swelling.
**Mitigation:** Battery voltage sense circuit (10k/3.3k divider to ADC). Firmware cuts motors and flashes LED at 6.4V (3.2V/cell). Also saves the end of a test run — LED warning gives time to land gracefully.

### 6. Motor Wiring Polarity Error
**Cause:** Motor connector plugged backward. One side drives in reverse. Robot spins in circles.
**Mitigation:** Polarity marked on PCB silkscreen. Test each motor independently with CMD:F,100 before full assembly. Fix by swapping motor cable wires, not in firmware.

### 7. I2C Bus Lockup
**Cause:** MPU-6050 holds SDA low after interrupted transaction. Bus frozen.
**Mitigation:** I2C bus recovery routine in firmware init (9 SCL clocks while SDA high). Runs before main loop.

### 8. UART Garbage on Pi Boot
**Cause:** Pi kernel uses GPIO14 (UART TX) for boot messages. Arrives at ESP32 as garbage bytes.
**Mitigation:** Firmware discards any packet without valid CMD: prefix. Also: disable serial console in raspi-config on Pi setup (required step, see vision README).

### 9. PWM Noise on Encoder/Sensor Lines
**Cause:** 20kHz PWM couples capacitively into parallel sensor wires. Adds noise to encoder counts and HC-SR04 echo timing.
**Mitigation:** 100nF cap on each encoder signal line to GND at ESP32 pin. Route sensor cables away from motor cables. Keep ultrasonic cables under 20cm.

### 10. Pi Thermal Throttling Under Inference
**Cause:** Pi 4 SoC reaches 80C without heatsink. Throttles from 1.8GHz to 1.5GHz (then 1.0GHz at 85C). Drops YOLO FPS. Slows following response.
**Mitigation:** Self-adhesive aluminum heatsink on Pi 4 SoC (BCM2711 chip). Reduces operating temp by 15-20C. If still throttling: 30mm 5V blower fan mounted on chassis directed at Pi.

### 11. HC-SR04 Simultaneous Firing (Crosstalk)
**Cause:** All three sensors fired at the same time. Sensor B hears Sensor A's pulse as a phantom reflection at ~0 cm. ESP32 registers a phantom wall directly in front and triggers the escape sequence every cycle.
**Mitigation:** Sequential sensor firing with 60ms minimum delay between each sensor. Never fire more than one sensor at a time. See firmware README for timing implementation.

### 12. TB6612FNG Overheating with JGA25-370
**Cause:** JGA25-370 stall current (1.5-2.2A at 7.4V) exceeds TB6612FNG continuous rating (1.2A). Sustained near-stall conditions cause IC to overheat.
**Mitigation:** Exposed thermal pad on TB6612FNG QFN package must be connected to GND copper pour in KiCad footprint. Firmware soft-start (PWM ramp over 200ms) reduces startup stall duration. Watchdog and obstacle avoidance prevent sustained stall. Run Test 2 and confirm ICs are warm but not hot.

### 13. Control Loop Oscillation
**Cause:** 200ms vision-to-motor latency combined with aggressive turn commands. Robot overshoots center, camera now sees person on opposite side, sends opposite turn command, overshoots again. Robot sways left-right following person.
**Mitigation:** Exponential moving average on offset value in Pi code (0.7 x previous + 0.3 x new). Dead zone ±0.15 already helps but is not sufficient alone. See vision README for implementation.

### 14. Pi/ESP32 State Desync
**Cause:** Pi sends STOP (enters WAITING state). ESP32 is mid-execution of last forward command. 100-300ms of "ghost motion" occurs — robot moves when it should be stopped.
**Mitigation:** Pi flushes serial write buffer before entering WAITING state. ESP32 watchdog at 500ms handles the case where Pi goes silent entirely. The residual 100-300ms desync is an accepted limitation of UART-based control without acknowledgment packets.

### 15. Ultrasonic Sensor Dropout Cascade
**Cause:** HC-SR04 returns no echo (object too close, sensor disconnected, electrical noise). Firmware interprets timeout as either 0cm (triggers escape loop constantly) or max range (treats path as clear when it is not).
**Mitigation:** Readings outside 2-400cm range flagged as INVALID. Firmware holds last known valid reading for up to 3 cycles (540ms) before reverting to safe default (treat as obstacle at 20cm). See firmware README.

### 16. Zero-Radius Spin Stalls on Carpet
**Cause:** During in-place spin, wheels skid laterally. Lateral friction coefficient on carpet (~0.65) is 4x higher than rolling resistance (0.15). Required torque per motor for spin: ~0.78 kg*cm vs safe continuous 0.45 kg*cm. Motors are above safe continuous rating during spin.
**Mitigation:** Never use zero-radius spin (full reversal on one side) on carpet. Use arc turns instead — one side slows to 40%, other at full speed. This produces a gradual turn without lateral wheel scrub. The firmware escape sequence (backing up then turning) is acceptable because the floor surface during backing is known to be open.

---

## PCB Design Checklist

**Schematic:**
- [ ] ESP32-WROOM-32 with 100nF + 10uF decoupling on VCC
- [ ] TB6612FNG x2 — all control pins connected, STBY tied HIGH
- [ ] 470uF bulk cap at each TB6612FNG VM pin (2 caps total)
- [ ] AMS1117-3.3 with 10uF + 100nF on input and output
- [ ] MP1584EN with feedback resistors for 5.0V output and 1000uF + 100nF output caps
- [ ] Reverse polarity MOSFET on VBAT input
- [ ] 7A-rated polyfuse on VBAT line (Littelfuse RGEF700 or equivalent — NOT a 5A part)
- [ ] AMS1117-3.3 input connected to 5V rail (MP1584EN output) — NOT to VBAT
- [ ] 1k/2k voltage dividers on all 3 HC-SR04 Echo lines (6 resistors)
- [ ] 4.7k I2C pull-ups on SDA and SCL to 3.3V
- [ ] Battery voltage sense divider (10k + 3.3k) to ESP32 ADC pin (GPIO36)
- [ ] Boot button (GPIO0 to GND)
- [ ] Reset button (EN to GND with 10k pull-up)
- [ ] Status LED with ~100 ohm current limiting resistor
- [ ] ERC clean — zero errors

**Layout:**
- [ ] Ground plane poured on bottom copper layer, net = GND
- [ ] Stitching vias every 10mm across board area
- [ ] Single star ground entry at XT30 negative terminal
- [ ] TB6612FNG exposed thermal pad connected to GND copper pour (NOT floating) — verify in KiCad footprint properties
- [ ] MP1584EN exposed thermal pad connected to copper pour on top layer
- [ ] 470uF bulk caps within 3mm of each TB6612FNG VM pin
- [ ] 1000uF cap within 5mm of MP1584EN output pin
- [ ] All 100nF decoupling caps within 2mm of IC power pins
- [ ] TB6612FNG placed adjacent to motor output connectors
- [ ] Motor output traces >= 1.2mm
- [ ] VBAT traces >= 2.5mm
- [ ] 5V RPi traces >= 1.5mm
- [ ] Encoder traces routed away from motor traces (opposite PCB side preferred)
- [ ] Polarity marked on motor connectors in silkscreen
- [ ] Board outline matches dimensions given to mechanical team
- [ ] DRC clean — zero errors
- [ ] Gerbers verified in viewer before upload

---

## Timeline

| Week | Target |
|---|---|
| 1-2 | Schematic complete — all blocks, all decoupling, all connectors, ERC clean |
| 2-3 | PCB layout with ground plane, DRC clean |
| 3 | Gerbers to JLCPCB, 5 boards ordered |
| 4-5 | PCB arrives, solder, power-on test |
| 5 | Run electrical stress test protocol before connecting Pi |
