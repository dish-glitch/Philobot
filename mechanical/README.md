# Mechanical — Philo Chassis

**Owner: [@hummos430](https://github.com/hummos430)**

---

## Overview

Philo uses a differential drive system — 4 wheels, split into a left side and a right side. Both wheels on the left are driven together, both wheels on the right are driven together. Turning is done by spinning the two sides at different speeds or in opposite directions. No steering axle, no servo, no complex geometry. This is the same drive system used on tanks and wheelchairs — simple, reliable, and capable of turning in place.

---

## How Turning Works

| Command | Left Side | Right Side | Result |
|---|---|---|---|
| Forward | Forward | Forward | Straight |
| Turn Right | Forward | Slower / Reverse | Curves or pivots right |
| Turn Left | Slower / Reverse | Forward | Curves or pivots left |
| Spin in Place | Forward | Reverse | Zero-radius turn |
| Stop | Stop | Stop | Brakes |

The ESP32 controls each side independently via PWM. hummos430 does not need to worry about the electronics — just make sure the left two wheels are mechanically linked to their motors and the right two to theirs.

---

## Weight Budget

This matters because the motors have to actually move the robot. Every gram added to the chassis is a gram the motors fight against.

| Component | Estimated Weight |
|---|---|
| Raspberry Pi 4 | 46g |
| 2S LiPo 2200mAh battery | 140g |
| PETG-CF chassis (all printed parts) | ~110g |
| 4x N20 gear motors | ~48g |
| 4x 65mm rubber wheels | ~50g |
| Custom PCB | ~40g |
| 3x HC-SR04 ultrasonic sensors | ~30g |
| Wiring and connectors | ~25g |
| Pi Camera + mount | ~8g |
| Screws, standoffs, hardware | ~20g |
| **Total (with margin)** | **~520g — design for 600g** |

**Design rule for hummos430:** Keep the printed chassis under 110g. If the chassis comes in heavier, reduce infill to 20% on non-structural panels or hollow out areas that do not carry load. Every 50g over budget reduces motor headroom.

---

## Motor Selection — Why 300 RPM, Not 200 RPM

The original spec said 200 RPM N20 motors. After running the numbers, **300 RPM is the better choice.**

**Speed comparison on 65mm wheels:**
- 200 RPM → 0.68 m/s top speed
- 300 RPM → 1.02 m/s top speed

A normal walking pace is 1.2–1.4 m/s. At 200 RPM, the robot can barely keep up with someone walking briskly — it would lag behind and lose the person. At 300 RPM there is enough headroom to follow comfortably and still have speed left over for corrections.

**Torque check — will 300 RPM motors actually move a 600g robot?**

Higher RPM N20 motors trade some stall torque for speed, typically 2–3 N·cm at 300 RPM.

Rolling resistance force (worst case — carpet):
```
F = rolling_resistance_coefficient × mass × gravity
F = 0.15 × 0.6kg × 9.81 = 0.88N
```

Torque needed at the wheel (per motor pair, 65mm wheel radius = 0.0325m):
```
torque = (F / 2 sides) × wheel_radius
torque = (0.88 / 2) × 0.0325 = 0.014 N·m = 1.4 N·cm
```

A 300 RPM N20 provides ~2–3 N·cm. We need 1.4 N·cm. **Passes with margin on carpet.** On smooth floors (wood, tile), rolling resistance drops to ~0.02 and the motors are barely loaded.

**Bottom line:** Order N20 motors at 12V, 300 RPM, with encoders. Confirm shaft is 3mm D-shaft.

---

## Center of Gravity

Where weight sits matters as much as how much there is.

**Battery placement:** The 2S LiPo at 140g is the single heaviest component. Mount it as low as possible and centered between the front and rear axles. If it is too far back, the robot tips backward when accelerating forward. If too far forward, it pitches forward when braking.

**Camera placement:** The camera sits elevated at the front — this adds a small top-heavy moment. Keep the mount as short as possible while still clearing the front of the chassis. 30–35mm above the chassis top is enough.

**PCB and Pi placement:** Mount flat and centered. The Pi (46g) and PCB (~40g) sitting in the middle keeps the CG centered side to side.

**General rule:** heavy things go low and center. Light things can go anywhere.

---

## Target Dimensions

| Parameter | Target |
|---|---|
| Footprint | ~200mm (L) x 150mm (W) |
| Chassis body height | ~60–80mm |
| Ground clearance | 12mm minimum |
| Wheel diameter | 65mm |
| Wheelbase (front to rear axle) | ~120mm |
| Track width (left to right wheel center) | ~130mm |

Ground clearance: 12mm minimum clears most carpet pile and small floor objects. Less than 10mm and the chassis will drag on thick rugs.

---

## What Needs to Be Designed in Fusion 360

### 1. Main Chassis Plate
Flat base plate. All components mount to this. Needs:
- 4x motor mount pockets (one per corner)
- 4x M3 holes for PCB standoffs (positions locked in with hardware team before printing)
- 4x M2.5 holes for Raspberry Pi standoffs (58mm x 49mm — standard Pi hole spacing, do not guess this measurement)
- Battery bay: cradle or strap mount, sized for 2S LiPo pouch (~70mm x 35mm x 20mm). Mount as low as possible, centered front-to-back.
- 3x slots or pockets on the front face for ultrasonic sensors (one center, one angled left ~30 degrees, one angled right ~30 degrees). HC-SR04 body is 45mm x 20mm x 15mm.
- Cable routing channels on underside to keep wires away from wheels

### 2. Motor Mounts
Brackets that hold each N20 motor at the corner of the chassis with the shaft pointing outward. Motor should sit low — wheel should be close to the ground plane, not elevated. Leave 0.5mm clearance around the motor body so it fits without forcing. M2 screw holes to secure the motor.

### 3. Wheel Hubs
If purchased wheels do not include a hub for a 3mm D-shaft, print one. Tolerance: 0.1mm clearance on the D-flat, 0.0mm on the round diameter (press fit). Print at 100% infill — this part takes torque directly.

### 4. Camera Mount
Front-center, elevated 30–35mm above chassis top. Camera should tilt slightly downward (~10–15 degrees) so it sees a standing person from feet to head at 1–3 meters. Holds the Pi Camera Module v2 board (25mm x 24mm, 4 mounting holes at M2, 21mm x 12.5mm spacing).

### 5. Top Cover (low priority)
A cosmetic cover that makes Philo look intentional rather than like a flat board of components. Design this after everything else works and fits. Week 7 or later.

---

## Print Settings

**Material:** PETG-CF (already on hand)

| Setting | Value |
|---|---|
| Nozzle temp | 240–250°C |
| Bed temp | 70–80°C |
| Infill — structural parts (motor mounts, wheel hubs) | 30–40% |
| Infill — chassis plate and covers | 20–25% |
| Wall perimeters | 4 minimum |
| Layer height | 0.2mm |
| Nozzle | Hardened steel required — CF is abrasive and will destroy brass nozzles |

Print wheel hubs and motor mounts at 40% infill minimum. These parts take real mechanical stress. The main chassis plate can be 20% — it is mostly flat and not under high load.

**Orientation:** Print motor mounts with the screw holes vertical (layer lines perpendicular to load direction). Layer lines are the weak direction — if the mount is printed sideways, it will crack along the layers when torque is applied.

---

## Weight Reduction Tips (if chassis comes in over 110g)

- Pocket the underside of the chassis plate (leave a grid of ribs, remove material between them)
- Reduce wall count on cosmetic covers from 4 to 3 perimeters
- Do not reduce walls or infill on motor mounts — those take real force
- A 2mm thick chassis plate with 20% infill and 4 walls is strong enough for this robot

---

## Coordination Points with Hardware Team

**Before printing the final chassis — these are hard dependencies:**

| Item | Why It Matters |
|---|---|
| PCB dimensions and M3 standoff hole positions | Standoff holes in the wrong place means reprinting |
| Motor type confirmed (shaft diameter, body dimensions) | Motor mount pocket dimensions depend on this |
| Battery dimensions confirmed | Battery bay sizing depends on this |
| HC-SR04 body dimensions confirmed | Sensor pocket sizing (should be 45mm x 20mm) |
| Raspberry Pi hole pattern confirmed | 58mm x 49mm — verify against actual Pi before printing |

Lock all of these in with the hardware team by end of Week 2. A chassis printed before these are confirmed will need reprinting.

---

## Timeline

| Week | Target |
|---|---|
| 1–2 | Sketch top-down layout, confirm motor type and dimensions, rough footprint in Fusion 360 |
| 2 | Coordinate PCB standoff positions with hardware team |
| 3 | Chassis v1 ready to print — all mounting positions confirmed |
| 4 | Chassis v1 printed, test-fit motors, PCB, and Pi |
| 5–6 | Revisions based on integration testing |
| 7 | Final chassis, cable management, camera mount complete |
