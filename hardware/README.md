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

**Back-EMF on motor reversal:** When a motor is commanded to reverse direction mid-spin, it briefly acts as a generator. This creates a voltage spike back into the VM pin. The TB6612FNG has internal flyback diodes on all output transistors that clamp this spike to VM. This is one advantage of the TB6612FNG over discrete H-bridge designs — the protection is built in. The 470uF bulk cap at each TB6612FNG VM pin (see Section: Ground Architecture and Decoupling) further absorbs these transients.

**Placement:** Both TB6612FNG ICs on the same side as motor output connectors. Motor output traces (AO1, AO2, BO1, BO2) minimum 1.2mm. Short and direct to connectors.

### Block 3 — Power Architecture

Power failures mid-demo are catastrophic. This block must be correct.

**Input: 2S LiPo via XT30**

Voltage range: 6.4V (discharged) to 8.4V (fully charged). 5A polyfuse on input line — trips at sustained ~5A, resets after cooldown. P-channel MOSFET for reverse polarity protection.

**3.3V Rail: AMS1117-3.3 LDO**

Powers: ESP32, MPU-6050, TB6612FNG logic, sensor pull-ups, LEDs.

Caps: 10uF electrolytic + 100nF ceramic on both input and output pins. Required for AMS1117 stability — without them the output oscillates at high frequency.

Max load: ~270mA. Rated 800mA. Adequate headroom.

**5V Rail: MP1584EN Buck Converter**

> Correction from earlier revision: MP2307 was originally specified. MP2307 is rated 1.2A. Pi 4 draws up to 3A under inference. That would have burned out the converter mid-demo. MP1584EN is rated 3A. This is the correct part.

Input: VBAT (6.4-8.4V)
Output: 5.0V (set via feedback resistor divider, per section 7 of MP1584EN datasheet)

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

Mount flat and horizontal on the PCB. 4.7kΩ pull-ups on SDA and SCL to 3.3V. ESP32 internal pull-ups are too weak for reliable I2C.

Implement I2C bus recovery routine in firmware (9 SCL clocks with SDA held high) to clear SDA-stuck-low lockups from Pi resets mid-transaction.

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
VBAT ---- 10k ----+---- ESP32 ADC (GPIO34 or GPIO35, input-only pins)
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
| 4x N20 motors (running load) | VBAT | 480mA total | 5.6A (reversal back-EMF transient, <50ms) | Motor spec + back-EMF analysis |
| ESP32 (active, WiFi off) | 3.3V | 240mA | 340mA | Espressif datasheet |
| 3x HC-SR04 | 5V | 45mA | same | HC-SR04 datasheet |
| MPU-6050 | 3.3V | 3.9mA | same | InvenSense datasheet |
| Misc (LEDs, pull-ups) | 3.3V | 20mA | same | Estimated |
| **Total normal operation** | | ~3.3A | | |
| **Motor reversal transient** | | | ~5.6A for <50ms | Absorbed by 470uF caps |
| **All motors stalled** | | ~6.0A sustained | | Polyfuse trips |

**Battery runtime:** 2200mAh x 7.4V = 16.3Wh. At 24.4W normal draw: ~40 minutes. Sufficient for demo and test runs.

**Why the transient number matters:** 5.6A for 50ms is 0.28 coulombs. A 470uF cap can supply 0.47 coulombs at 1V sag. Two 470uF caps (one per TB6612FNG) combined handle this without the battery seeing any significant transient. This is why the bulk caps are non-optional.

---

## Trace Width Requirements

| Net | Max Current | Minimum Width | Notes |
|---|---|---|---|
| VBAT input | 6A peak | 2.5mm | Size for fault condition |
| Motor outputs (per TB6612FNG channel) | 1.2A cont, 3.2A peak | 1.2mm | Short, direct to connector |
| 5V RPi rail | 3A | 1.5mm | Full Pi inference current |
| 3.3V logic rail | 500mA | 0.8mm | Include margin |
| Signal traces (UART, I2C, GPIO) | <50mA | 0.25mm | KiCad default |

---

## Electrical Stress Test Protocol

Before trusting any demo, run these tests in order. If the robot passes all four, the power system is solid.

**Test 1 — Full throttle forward for 30 seconds**
Command CMD:F,255. Measure battery voltage at start and end. Should not sag more than 0.3V. Feel the TB6612FNG ICs and MP1584EN — should be warm but not hot.

**Test 2 — Repeated hard reversal**
Alternate CMD:F,255 and CMD:B,255 every 2 seconds, 20 times. Watch for ESP32 resets or Pi undervoltage icon. None should occur. This is the worst case for back-EMF transients.

**Test 3 — Stall one wheel**
Block one wheel by hand for 3 seconds. Robot should not reset. Polyfuse should not trip (single motor stall is ~700mA, well under 5A). Release and confirm motors spin up correctly.

**Test 4 — Full system under inference**
Run full YOLO stack on Pi while robot follows. Watch Pi temperature (vcgencmd measure_temp). Target under 70C with heatsink. If throttling occurs (check with vcgencmd get_throttled), add a 30mm fan.

---

## Failure Mode Analysis

### 1. Motor Burn from Sustained Stall
**Cause:** Wheel jammed, motor at stall current (700mA) indefinitely. N20 motor overheats in under 2 minutes.
**Mitigation:** 5A polyfuse trips on full 4-motor stall. Watchdog stops motors within 500ms of lost Pi comms. Obstacle escape sequence prevents sustained stall in normal operation.

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

---

## PCB Design Checklist

**Schematic:**
- [ ] ESP32-WROOM-32 with 100nF + 10uF decoupling on VCC
- [ ] TB6612FNG x2 — all control pins connected, STBY tied HIGH
- [ ] 470uF bulk cap at each TB6612FNG VM pin (2 caps total)
- [ ] AMS1117-3.3 with 10uF + 100nF on input and output
- [ ] MP1584EN with feedback resistors for 5.0V output and 1000uF + 100nF output caps
- [ ] Reverse polarity MOSFET on VBAT input
- [ ] 5A polyfuse on VBAT line
- [ ] 1k/2k voltage dividers on all 3 HC-SR04 Echo lines (6 resistors)
- [ ] 4.7k I2C pull-ups on SDA and SCL to 3.3V
- [ ] Battery voltage sense divider (10k + 3.3k) to ESP32 ADC pin
- [ ] Boot button (GPIO0 to GND)
- [ ] Reset button (EN to GND with 10k pull-up)
- [ ] Status LED with ~100 ohm current limiting resistor
- [ ] ERC clean — zero errors

**Layout:**
- [ ] Ground plane poured on bottom copper layer, net = GND
- [ ] Stitching vias every 10mm across board area
- [ ] Single star ground entry at XT30 negative terminal
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
