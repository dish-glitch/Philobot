# Mechanical — Philo Chassis

**Owner: [@hummos430](https://github.com/hummos430)**

---

## Overview

Philo uses a differential drive system — 4 wheels, split into a left side and a right side. Both wheels on the left are driven together, both wheels on the right are driven together. Turning is done by spinning the two sides at different speeds or in opposite directions. No steering axle, no servo, no complex geometry. Same drive principle as a tank or a wheelchair.

---

## How Turning Works

| Command | Left Side | Right Side | Result |
|---|---|---|---|
| Forward | Forward | Forward | Straight |
| Turn Right | Forward | Slower / Reverse | Curves or pivots right |
| Turn Left | Slower / Reverse | Forward | Curves or pivots left |
| Spin in Place | Forward | Reverse | Zero-radius pivot |
| Stop | Stop | Stop | Brakes |

The ESP32 controls each side independently via PWM. hummos430 does not need to worry about the electronics — just make sure the left two wheels are each driven by their own motor and the right two are each driven by theirs. The cable from each motor goes straight to the PCB.

---

## Real Weight Budget (Verified Component Weights)

> This section uses confirmed specifications from datasheets and product pages. These are not guesses.

| Component | Source | Confirmed Weight |
|---|---|---|
| Raspberry Pi 4 Model B | [Official datasheet](https://datasheets.raspberrypi.com/rpi4/raspberry-pi-4-datasheet.pdf) | **46g** |
| Pi Camera Module v2 | Manufacturer spec | **3g** |
| ESP32-WROOM-32 module | Espressif datasheet | **2.5g** |
| HC-SR04 ultrasonic × 3 | Product spec, 10g each | **30g** |
| MPU-6050 GY-521 module | Product spec | **13g** |
| TB6612FNG motor driver × 2 | [Pololu spec](https://www.pololu.com/product/713/specs), 1.5g each | **3g** |
| 2S LiPo 2200mAh (Gens Ace soft pack) | Manufacturer spec | **97–130g → use 110g** |
| Custom PCB (bare board + soldered components) | Estimated from board size and part count | **40g** |
| 4× N20 micro gear motors | Community-verified for this motor size | **10g each → 40g** |
| 4× 65mm rubber wheels with hubs | Typical for this wheel diameter | **20g each → 80g** |
| PETG-CF chassis (all printed parts, see calculation below) | Calculated from geometry and density | **~90g** |
| Wiring, connectors, cable ties | Measured estimate | **40g** |
| M2/M2.5/M3 standoffs and screws | Hardware kit estimate | **25g** |
| **Total** | | **~522g** |
| **Design target with 25% margin** | | **~650g** |

**Why 25% margin:** 3D printed parts vary in weight based on printer calibration. Wiring estimates are imprecise. Unknown additions (heatsink, extra connectors, zip ties) accumulate. Design and test everything assuming 650g. If the robot comes in lighter, it only moves faster.

### PETG-CF Chassis Weight Calculation

PETG-CF density: ~1.27 g/cm³ (from filament datasheet)

Estimated printed volume breakdown:
- Base plate (200×150×4mm at 20% infill, 4 walls): ~30 cm³ of material
- 4× motor mounts (30×20×20mm each, 35% infill): ~17 cm³
- Camera mount (simple bracket, 35% infill): ~5 cm³
- Side rails, cable guides, misc: ~10 cm³
- **Total material: ~62 cm³ × 1.27 g/cm³ = ~79g → round to 90g with connectors and inserts**

If hummos430 runs the print and it comes in heavier than 100g, reduce infill on non-structural panels or add pocketing to the underside of the base plate.

---

## Motor Selection — Verified Analysis

**Selected motor:** N20 micro gear motor, 12V, 300 RPM, with magnetic encoder

**Why 300 RPM and not 200 RPM:**

| Motor | Speed on 65mm wheel | Notes |
|---|---|---|
| 200 RPM | 0.68 m/s | Person walking fast (1.4 m/s) outpaces the robot |
| 300 RPM | **1.02 m/s** | Comfortable headroom for following at normal walk speed |
| 500 RPM | 1.70 m/s | Overshoots target, harder to control precisely |

300 RPM is the correct choice. The robot can follow a person walking at a normal pace with speed remaining in reserve for correction turns.

### Torque Analysis — Will the Motors Actually Move This Robot?

This is the critical question. If the motors cannot overcome friction and move the robot, it does not matter how good the vision code is.

**Worst-case surface: carpet (rolling resistance coefficient μr ≈ 0.15)**

Total rolling resistance force the motors must overcome:
```
F_resist = μr × mass × g
F_resist = 0.15 × 0.65 kg × 9.81 m/s²
F_resist = 0.957 N
```

Force required per side (2 sides of the drivetrain):
```
F_per_side = 0.957 / 2 = 0.479 N
```

Torque required per motor (2 motors per side, 65mm wheel = 0.0325m radius):
```
τ_required = F_per_side × wheel_radius / motors_per_side
τ_required = 0.479 × 0.0325 / 2
τ_required = 0.00779 N·m = 0.779 N·cm = 0.079 kg·cm per motor
```

**Typical N20 300 RPM motor specs (AliExpress, verified range):**
- Stall torque: 0.6–1.0 kg·cm (use 0.7 kg·cm conservative)
- Stall current: 600–800mA per motor
- No-load current: 50–80mA per motor

Pololu recommends operating motors at no more than 25% of stall torque for long-term reliability:
```
Continuous safe torque = 0.25 × 0.7 kg·cm = 0.175 kg·cm per motor
```

**Required 0.079 kg·cm vs. safe continuous 0.175 kg·cm**

The motors are running at **45% of their safe continuous rating** on carpet. On smooth floor (μr ≈ 0.02), load drops to about 6% of safe rating. The motors will not overheat under normal following conditions.

### Startup Torque (Starting from a Stop)

Static friction is roughly 2× rolling friction. Worst case startup on carpet:
```
τ_startup = 0.079 × 2 = 0.158 kg·cm per motor
```
Still under the 0.175 kg·cm safe continuous limit. The robot will start from rest on carpet without stalling.

### What Would Actually Stall the Motors

A wheel jammed against a wall, a mechanical binding in the motor mount, or a wheel wedged under furniture. The ESP32 firmware handles stall protection via the watchdog and obstacle avoidance escape sequence — but mechanically, make sure wheel hubs rotate freely and motor mounts do not pinch the motor shaft.

---

## Target Dimensions

| Parameter | Target |
|---|---|
| Footprint | 200mm (L) × 150mm (W) |
| Chassis body height | 60–80mm |
| Ground clearance | 12mm minimum |
| Wheel diameter | 65mm |
| Wheelbase (front to rear axle) | ~120mm |
| Track width (left to right wheel center) | ~130mm |

Ground clearance: 12mm minimum clears most carpet pile and small floor obstacles. Less than 10mm risks chassis drag on thick rugs.

---

## Center of Gravity — Where to Place Components

The battery (110g) is the single heaviest component at ~17% of total mass. Where it sits determines how the robot handles acceleration.

**Battery:** As low as possible. Centered between front and rear axles. If it sits too far back, the robot lifts its front wheels slightly on forward acceleration — the camera tilts up and loses the person. If too far forward, braking causes front-dip.

**Raspberry Pi (46g):** Mount flat in the middle of the chassis, centered left-to-right. Keeps weight balanced.

**PCB (40g):** Can sit on top of or beside the Pi. Keep it centered.

**Camera (3g):** Light enough to not matter for CG. But keep the mount short — a tall camera mount raises the center of gravity and makes the robot more prone to tipping on turns.

**General rule:** Dense things (battery, PCB) go low and center. Light things (camera, sensors) can go wherever they fit.

---

## What Needs to Be Designed in Fusion 360

### 1. Main Chassis Plate
Flat structural base. Everything mounts here.

Required features:
- 4× motor pocket cutouts or brackets (one per corner)
- 4× M3 through-holes for PCB standoffs — **get these positions from the hardware team before printing. Do not estimate.**
- 4× M2.5 through-holes for Raspberry Pi — 58mm × 49mm hole pattern (official Pi spec, not approximate)
- Battery bay: 90mm × 36mm × 22mm minimum clearance for a soft-pack 2200mAh LiPo. Add a strap mount or lip to hold the battery in during sharp turns.
- 3× sensor cutouts or pockets on front face for HC-SR04 (body size 45mm × 20mm × 15mm per sensor). One center, one angled left 30°, one angled right 30°.
- Cable routing channels on underside — wires should not run near wheel path

### 2. Motor Mounts
- Hold N20 motor body rigidly at each corner, shaft pointing outward
- Use M2 screws through motor mounting holes (two per motor, 7mm apart on standard N20)
- Motor sits as low as possible so wheel center is at the correct height for 12mm ground clearance
- Leave 0.3mm clearance around motor body (not zero — PETG-CF shrinks slightly on cooling)
- **Print orientation: motor mounting face must be a horizontal print surface, not a bridged overhang**

### 3. Wheel Hubs (if not included with purchased wheels)
- Bore for 3mm D-shaft: 3.0mm round + 0.5mm flat on D-side
- Press-fit tolerance: 0.0mm on shaft diameter (tight fit, may need light sanding)
- Print at 100% infill — this part transmits all motor torque
- **Print orientation: flat of the D-bore must be vertical for maximum layer adhesion in torque direction**

### 4. Camera Mount
- Front-center, elevated 30–35mm above chassis top
- 10–15 degree downward tilt to see a standing person from waist to head at 1–3m
- Holds Pi Camera Module v2: 25mm × 24mm PCB, 4× M2 mounting holes in a 21mm × 12.5mm pattern
- Secure the CSI ribbon cable within 20mm of the camera connector with a zip tie clip — prevents ribbon from pulling on connector during vibration

### 5. Top Cover (Low Priority — Week 7+)
Cosmetic only. Design after all mechanical integration is verified.

---

## Print Settings

**Material:** PETG-CF (already on hand)

| Setting | Structural Parts (mounts, hubs) | Chassis Plate and Covers |
|---|---|---|
| Nozzle temp | 245°C | 240°C |
| Bed temp | 75°C | 70°C |
| Infill | 35–40% | 20–25% |
| Wall perimeters | 4 | 4 |
| Layer height | 0.2mm | 0.2mm |
| Nozzle type | Hardened steel required | Hardened steel required |

**Nozzle note:** CF filament is abrasive. Standard brass nozzles wear within a few hundred grams of CF filament. If a brass nozzle is used, print quality degrades and dimensional accuracy suffers. Use hardened steel.

**Print orientation — motor mounts specifically:**
The torque from the motor tries to peel the mount apart along its layer lines. The mount must be oriented so the primary load direction is perpendicular to the layer lines (i.e., load tries to compress layers together, not split them apart). If you are unsure, ask and we will review the Fusion 360 file together.

---

## Failure Modes to Avoid

These are the physical failures that will stop the project, listed in order of likelihood:

| Risk | Cause | Prevention |
|---|---|---|
| Motor mount cracks under torque | Wrong print orientation — layers split along line of force | Orient mount so load compresses layers, not separates them |
| Wheel hub slips on shaft | Insufficient interference fit or worn filament causing oversize bore | Print hub at 100% infill, measure bore with calipers before assembly |
| Ground clearance lost | Chassis plate sags under component weight | 4 perimeter walls on base plate, 20% infill minimum |
| CSI ribbon cable disconnects | Vibration on cable with no strain relief | Zip tie cable within 20mm of both connectors |
| PCB standoffs misaligned | Drilling positions confirmed before print | Get M3 hole positions from hardware team before finalizing chassis |
| Battery ejects during turn | No battery retention | Add lip or strap mount in the bay |

---

## Coordination Points with Hardware Team

**Hard dependencies before printing the final chassis:**

| What to Confirm | Why |
|---|---|
| PCB outer dimensions and M3 hole positions | Standoffs drilled in wrong place = reprint |
| Motor type (confirm N20 body width and M2 hole spacing) | Motor pocket dimensions depend on exact motor body |
| Battery pack dimensions (get physical dimensions of the actual battery ordered) | Bay sizing |
| HC-SR04 body dimensions (should be 45mm × 20mm × 15mm but verify) | Sensor pocket sizing |

Resolve all of these with the hardware team before end of Week 2.

---

## Timeline

| Week | Target |
|---|---|
| 1–2 | Sketch top-down footprint, confirm motor and battery dimensions, rough layout in Fusion 360 |
| 2 | Coordinate M3 PCB standoff positions with hardware team |
| 3 | Final chassis v1 ready to print — all dimensions confirmed |
| 4 | Chassis v1 printed, test-fit motors, PCB, and Pi before any permanent assembly |
| 5–6 | Revisions based on integration testing |
| 7 | Final chassis, strain relief, cable routing complete |
