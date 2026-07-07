# Philo — Pre-Build Lock List

**Purpose:** This is the design freeze document. Before Gerbers go to JLCPCB and chassis printing begins, this file defines what is FROZEN (changing it costs a PCB respin or chassis reprint), what is TUNABLE (dial in during testing, no commitment needed now), and which upgrades are worth doing versus which are overengineering to ignore.

**Read this before the Week 3 PCB order. Once the board is fabricated, the FROZEN section is locked.**

---

## How to Use This Document

A design freeze is the point where you stop improving and start building. Past this point:
- Changing a FROZEN item means re-spending money (PCB ~$15 + 10 days, or a chassis reprint of several hours)
- Changing a TUNABLE item is just editing code or a slicer setting — costs nothing

The discipline is: **resist the urge to keep "improving" frozen things.** Every late change to a frozen item is a chance to introduce a new bug into a system that was working. Improvements go into a V2 list, not into the board you are about to order.

---

## SECTION A — FROZEN (do not change after PCB order / chassis print)

These decisions are load-bearing. Other parts of the design are built around them. Changing one cascades into reprints, respins, or reorders.

### A1 — Motor: JGA25-370, 6V, 200 RPM, 4mm D-shaft
**Why frozen:** Everything depends on this. Wheel hub bore (4mm D), motor mount pocket (25mm body), torque math (1.8 kg·cm stall), motor mount hole spacing, and the entire weight/power budget are sized to this exact motor.
**Confirm before freeze:** hummos430 must measure the *actual ordered motor* with calipers — body diameter, shaft diameter, and mounting hole spacing — and confirm they match the spec. Order the motors FIRST so this can be verified before the chassis is finalized.

### A2 — Top speed is locked at ~0.52 m/s by this motor choice — decide NOW if that's enough
**This is the one freeze decision people regret.** The JGA25-370 is a 6V motor. At 2S (7.4V) it already runs 23% over rated voltage — acceptable. Going to 3S (11.1V) for more speed would run it 85% over rated, which shortens motor life badly and is not a real option for *this* motor.

So the honest situation: **with these motors, 2S is correct and 0.52 m/s is your ceiling.** If 0.52 m/s (a casual walk the person slows down for) is not acceptable for your demo, the fix is a *12V-rated motor*, which is a motor change — and a motor change must happen BEFORE the chassis and PCB are frozen because it changes shaft size, mount dimensions, and current budget.

**Decision required now:** Accept 0.52 m/s with JGA25-370, OR switch to a 12V-rated gear motor before freeze. Do not discover this gap after the chassis is printed.

### A3 — PCB outer dimensions and mounting hole positions
**Why frozen:** The chassis plate is designed with cutouts and standoff holes that match the board exactly. The board outline in KiCad must match the dimensions handed to hummos430. If the board grows by 5mm after the chassis is printed, it won't fit.
**Locked:** Board grew from the original 100×100mm to **116×116mm** to make room for 4x real M3 mounting holes (the original layout had no board-edge clearance for them). Hole coordinates are in `mechanical/README.md` section "1. Main Chassis Plate." DRC and ERC both verified clean (0 errors) after the resize.
**Confirm before freeze:** Hardware team and hummos430 sign off that the chassis plate is designed against 116×116mm and the exact hole coordinates in `mechanical/README.md`, not the old 100×100mm figure.

### A4 — ESP32 GPIO pin assignments
**Why frozen:** Once the PCB is routed, the pins are physically wired. The firmware adapts to the board, not the other way around. We already resolved the GPIO 21/22 double-assignment conflict (I2C vs ultrasonic). That fix is locked.
**Locked map:** Motor PWM/dir on 25/26/27 and 32/33/14, encoders on 34/35, ultrasonic triggers on 5/13/19, echoes on 4/18/23, I2C on 21/22 (now carries MPU-6050 + 2 OLED eyes — no new pins), UART on 16/17, battery ADC on 36, status LED on 2. Verify against the KiCad schematic before routing. Note: the ESP32 is essentially full — the only remaining free pins (0, 12, 15) are boot-strapping pins, which is why audio lives on the Pi, not here.

### A5 — Power architecture
**Why frozen:** This is the part that took the most review to get right. Locked:
- 5V rail: Pololu D24V50F5 buck (5V/5A, fixed output), 1000µF + 100nF output caps. Upgraded from the 3A MP1584EN for headroom on the Pi rail (Pi 2.5-3A + AMS cascade + possible USB speaker)
- 3.3V rail: AMS1117-3.3 fed **from the 5V rail, not VBAT** (thermal fix — 62°C vs 135°C junction)
- 7A-rated polyfuse on VBAT input (not 5A — avoids pre-trip voltage sag)
- Reverse-polarity P-MOSFET on VBAT
- TB6612FNG ×2, thermal pad to GND copper pour
- 470µF bulk cap at each TB6612FNG VM pin
- Battery voltage sense divider (10k/3.3k) to GPIO36

### A6 — Ground architecture
**Why frozen:** Solid ground plane on bottom layer, via stitching every 10mm, single star entry at XT30 negative. This is a layout decision baked into the board. Cannot be added after fabrication.

### A7 — Connectors and battery form factor
**Why frozen:** XT30 battery connector and the 2S 2200mAh soft-pack form factor define the battery bay dimensions in the chassis. Motor and sensor headers define footprint placement.
**Confirm before freeze:** Get the *physical dimensions of the actual battery ordered* — soft-pack sizes vary. Size the bay to the real pack, not the nominal spec.

### A8 — Camera mast height (380mm total) and tilt (20–25° upward)
**Why frozen:** This is the geometry that makes gesture detection physically possible. The chassis mast-mounting point is designed for this height. The vision system's keypoint visibility math assumes it. Lower the mast and the camera sees knees instead of arms — gesture detection stops working entirely.

### A9 — HC-SR04 echo voltage dividers (1k/2k) and the level-shift requirement
**Why frozen:** These protect the ESP32 GPIO from 5V echo signals. 6 resistors on the schematic. Not optional, not changeable post-fab.

### A10 — Companion eyes + audio architecture (added at design freeze)
**Why frozen:** Decided before the board and chassis are locked, so the I2C bus and chassis cutouts can be designed in now rather than retrofitted.
- **Eyes:** 2x SSD1306/SH1106 OLED (128x64, I2C) at addresses 0x3C and 0x3D, on the *existing* I2C bus alongside the MPU-6050 (0x68). Three I2C devices total. No new ESP32 GPIO pins. Driven by the ESP32 on **core 1** (a full refresh is ~23ms and must not run in the control loop on core 0).
- **Audio:** driven by the **Raspberry Pi**, not the ESP32 (the ESP32 has no safe free pins left — only boot-strapping pins remain). Pre-recorded WAV clips via aplay, triggered on state transitions. The custom PCB carries audio only as a 5V power allowance if the speaker taps the 5V rail.
- **Chassis impact (hummos430):** two OLED-sized cutouts in the front of the mast head (active area ~22x11mm, module ~27x27mm) and a speaker grille. These must be in the chassis design before printing.
- **Live ChatGPT voice = explicitly V2.** Not in this build (see D7).

---

## SECTION B — TUNABLE (leave open, dial in during integration/testing weeks)

Do NOT try to perfect these before the build. They are software values or slicer settings. You will tune them on the real robot in weeks 5–8, and the real robot will tell you the right values faster than any amount of planning.

| Parameter | Starting value | Where it's tuned |
|---|---|---|
| PID gains | Kp=1.0, Ki=0.1, Kd=0.05 | Week 6, on the bench with wheels off the ground first |
| Vision dead zone | ±0.15 | Week 7, watching follow behavior |
| Offset EMA alpha | 0.3 | Week 7, if oscillation appears |
| Bbox height distance thresholds | 0.2 / 0.5 / 0.7 | Week 7, with real person at known distances |
| Inference image size | imgsz=320 (try 224) | Week 5, benchmark FPS vs detection range |
| Soft-start ramp time | 200ms | Week 6, watching motor startup current |
| Ultrasonic stop/slow thresholds | 25cm / 30cm | Week 7, in the actual demo room |
| Ultrasonic scan gap | 60ms | Week 6, confirm no crosstalk |
| Watchdog timeout | 500ms | Week 7, balance safety vs responsiveness |
| Gesture debounce frames | 5 | Week 6, balance responsiveness vs false triggers |
| Follow speed values | 150 turn / variable forward | Week 7 |
| Print infill on non-structural parts | 20–25% | Whenever, based on weight |

**Rule:** if it lives in code or the slicer, it is not a freeze decision. Stop debating these now.

---

## SECTION C — WORTH-IT UPGRADES (small additions, real payoff)

These are cheap, low-risk, and genuinely improve the build. Add them to the PCB/build NOW while it's free to do so — most cost nothing once the board is being designed anyway.

### C1 — Test points on the PCB ⭐ (highest value, near-zero cost)
Add labeled test points (just exposed pads or small header pins) for: VBAT, 5V, 3.3V, GND (×2), UART TX, UART RX, and one ESP32 GPIO. When something doesn't work during bring-up — and something always doesn't — these turn a multi-hour debugging session into a 30-second probe. This is the single best thing you can add to a first-revision board.

### C2 — 10µF ceramic cap at the XT30 input
A ceramic responds faster than the bulk electrolytic to the initial edge of a motor transient. One part, a few cents, slightly cleaner VBAT. Worth it.

### C3 — Aluminum heatsink on the Pi BCM2711 SoC
Already in the failure-mode list (#10). Confirm it's actually in the build, not just on paper. Self-adhesive, ~$2, prevents thermal throttling that would drop your YOLO FPS mid-demo.

### C4 — Break out 2–3 spare ESP32 GPIOs to a header
Costs nothing on the board. If you discover during integration that you need one more sensor or a debug LED, you have pins available without a respin.

---

## SECTION D — OVERENGINEERING (do NOT do these — they add risk or scope, not value)

Equally important as knowing what to add: knowing what to refuse. Each of these has been considered and rejected for good reason.

### D1 — ❌ LC filter on the motor rail
Suggested in review. **Do not do this.** An inductor in series with the motor current path fights the current change during reversal and generates a voltage spike that can exceed what the TB6612FNG flyback diodes and bulk caps handle. It makes the transient problem *worse*, not better. The 470µF bulk caps are the correct solution.

### D2 — ❌ "Power domain separation" beyond what exists
Review flagged this as a missing upgrade. It isn't missing — we already have it. The D24V50F5 is the dedicated logic buck, motors run directly from VBAT through the TB6612FNG, bulk caps isolate the transient domains. The separation being asked for is already in the design.

### D3 — ❌ Coral USB / Hailo / Jetson for faster inference
The ~200ms latency ("laggy dog" feel) is real but is an accepted characteristic, not a defect. Adding an AI accelerator is a major scope, cost, and integration change for a demo that works fine at current speed. V2 territory.

### D4 — ❌ A 4th ultrasonic sensor / wider sensor array
The ±15–30° side gap is acceptable for a living-room demo where obstacles are walls, furniture edges, and table legs approaching from the front. More sensors means more crosstalk timing complexity and more board space for marginal benefit.

### D5 — ❌ Person re-identification / shirt-color tracking
"Follow the largest bounding box" is the V1 approach and it's documented with its limitation. True re-ID is a hard research problem. Don't open it for this timeline.

### D6 — ❌ SLAM / mapping
Already rejected earlier in the project. Reactive obstacle avoidance is the right scope. Don't reintroduce it.

### D7 — ❌ Live ChatGPT voice conversation (for V1)
A real voice pipeline (mic → speech-to-text → LLM API → text-to-speech → speaker) is a second project. It competes with YOLOv8 for the Pi's already-maxed CPU, adds a microphone hardware chain, and makes the demo depend on a live internet connection and external API uptime. The companion feel is delivered by eyes + pre-recorded clips (A10) at near-zero risk. Live conversation is a post-demo V2 goal. To keep the door open cheaply: leave one Pi USB port free for a future mic. Build nothing else for it now.

### D8 — ❌ 4S battery / drivetrain redesign (for V1)
A 4S pack (16.8V full charge) exceeds the TB6612FNG absolute max (15V) and would destroy the 6V JGA25-370 motors. Using it requires 12V motors + a higher-voltage driver (or a motor-rail buck) + a larger chassis — a 1–2 week redesign of frozen items. Decided against for V1; a ~$15 2S 2200mAh pack keeps the proven design intact. The 4S is a V2 option if a bigger/faster build is pursued later.

---

## SECTION E — Freeze Sign-Off Checklist

Before the board is ordered (target: Week 3), confirm every item:

- [ ] Motors ordered and physically measured (A1) — caliper-verified body, shaft, hole spacing
- [ ] Speed decision made and accepted (A2) — 0.52 m/s with JGA25-370, OR motor swapped before freeze
- [ ] PCB outline and standoff holes agreed in writing between hardware and hummos430 (A3)
- [ ] ESP32 pin map verified against KiCad schematic, no conflicts (A4) — OLED eyes use existing I2C, no new pins
- [ ] Power architecture matches A5 exactly (AMS1117 from 5V, 7A fuse, thermal pads)
- [ ] Ground plane + star ground in the layout (A6)
- [ ] Actual battery measured, bay sized to it (A7) — 2S 2200mAh confirmed, NOT the 4S
- [ ] Camera mast geometry locked at 380mm / 20–25° (A8)
- [ ] HC-SR04 dividers on all 3 echo lines (A9)
- [ ] OLED eyes at 0x3C/0x3D on I2C bus; mast-head eye cutouts in chassis design (A10)
- [ ] Speaker grille in chassis design; speaker powered from Pi USB or 5V rail with budget noted (A10)
- [ ] Test points added (C1)
- [ ] 10µF ceramic at XT30 (C2)
- [ ] Pi heatsink in the build (C3)
- [ ] Spare GPIOs broken out to header (C4)
- [ ] ERC clean, DRC clean, Gerbers viewed before upload

**When every box is checked, the design is frozen. Order the board. Stop changing things. Start building.**
