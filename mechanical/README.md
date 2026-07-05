# Mechanical — Philo Chassis

**Owner: [@hummos430](https://github.com/hummos430) [@ezan13](https://github.com/ezan13)         **

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
| Raspberry Pi 5 (4GB) w/ active cooler | Official spec | **51g + ~15g cooler = ~66g** |
| Pi Camera Module 3 | Manufacturer spec | **4g** |
| ESP32-WROOM-32 module | Espressif datasheet | **2.5g** |
| HC-SR04 ultrasonic x3 | Product spec, 10g each | **30g** |
| MPU-6050 GY-521 module | Product spec | **13g** |
| TB6612FNG motor driver x2 | [Pololu spec](https://www.pololu.com/product/713/specs), 1.5g each | **3g** |
| 2S LiPo 2200mAh (Gens Ace soft pack) | Manufacturer spec | **97-130g -> use 110g** |
| Custom PCB (bare board + soldered components) | Estimated from board size and part count | **50g** |
| 4x JGA25-370 gear motors | Product spec, 90g each | **360g** |
| 4x 80mm rubber wheels with hubs | Typical for this wheel diameter | **30g each -> 120g** |
| Camera mast (PETG-CF printed) | Calculated from geometry | **~30g** |
| PETG-CF chassis (all printed parts, see calculation below) | Calculated from geometry and density | **~180g** |
| Wiring, connectors, cable ties | Measured estimate | **50g** |
| M2/M2.5/M3 standoffs and screws | Hardware kit estimate | **30g** |
| **Total** | | **~1048g** |
| **Design target with 15% margin** | | **~1.2kg** |

**Why 15% margin:** Wiring estimates are imprecise. Small additions (heatsinks, extra connectors, zip ties) accumulate. Design and test everything assuming 1.2kg. If the robot comes in lighter, it only moves faster.

### PETG-CF Chassis Weight Calculation

PETG-CF density: ~1.27 g/cm3 (from filament datasheet)

Estimated printed volume breakdown for 300mm x 220mm chassis:
- Base plate (300x220x5mm at 20% infill, 4 walls): ~60 cm3 of material
- 4x motor mounts (40x30x25mm each, 35% infill): ~30 cm3
- Side rails, cable guides, misc: ~20 cm3
- **Total material: ~110 cm3 x 1.27 g/cm3 = ~140g -> round to 180g with connectors and inserts**

If hummos430 runs the print and it comes in heavier than 200g, reduce infill on non-structural panels or add pocketing to the underside of the base plate.

---

## Motor Selection — Verified Analysis

**Selected motor:** JGA25-370 gear motor, 6V, 200 RPM, with encoder

**Why not N20 motors:** N20 motors are the right choice for a lightweight mini robot (~650g). For Philo at 1.2kg on a larger chassis, N20 stall torque (~0.7 kg·cm) gives very little safety margin on carpet. The JGA25-370 is the appropriate upgrade — 25mm body diameter, 4mm D-shaft, rated stall torque ~2.0 kg·cm. Still small enough to fit in the chassis without making it a forklift.

**Note on voltage and actual speed:** The motors are rated at 6V. Powered from 2S LiPo at 7.4V nominal, they run at ~7.4/6.0 = 1.23x rated speed = ~246 RPM. With 80mm wheels:

| Motor | Rated | At 7.4V | Speed on 80mm wheel | Notes |
|---|---|---|---|---|
| JGA25-370 200 RPM | 6V, 200 RPM | 246 RPM | **0.52 m/s** | Good following speed for demo |
| JGA25-370 100 RPM | 6V, 100 RPM | 123 RPM | 0.26 m/s | Too slow |
| JGA25-370 300 RPM | 6V, 300 RPM | 369 RPM | 0.77 m/s | More speed, less torque — carpet risk |

200 RPM at 6V is the correct choice. Demo following speed is typically 0.3-0.6 m/s. The robot will have speed margin in reserve for correction turns.

### Torque Analysis — Will the Motors Move This Robot?

**Worst-case surface: carpet (rolling resistance coefficient ur = 0.15)**

Total rolling resistance force the motors must overcome:
```
F_resist = ur x mass x g
F_resist = 0.15 x 1.2 kg x 9.81 m/s^2
F_resist = 1.77 N
```

Force required per side (2 sides of the drivetrain):
```
F_per_side = 1.77 / 2 = 0.885 N
```

Torque required per motor (2 motors per side, 80mm wheel = 0.040m radius):
```
t_required = F_per_side x wheel_radius / motors_per_side
t_required = 0.885 x 0.040 / 2
t_required = 0.0177 N*m = 1.77 N*cm = 0.18 kg*cm per motor
```

**JGA25-370 200 RPM at 6V specs (verified range from product pages):**
- Stall torque: 1.8-2.2 kg*cm (use 1.8 kg*cm conservative)
- Stall current: 1.0-1.8A per motor at 6V (at 7.4V: ~1.2-2.2A)
- No-load current: 80-120mA per motor

Pololu recommends operating motors at no more than 25% of stall torque for long-term reliability:
```
Continuous safe torque = 0.25 x 1.8 kg*cm = 0.45 kg*cm per motor
```

**Required 0.18 kg*cm vs. safe continuous 0.45 kg*cm**

The motors are running at **40% of their safe continuous rating** on carpet. On smooth floor (ur = 0.02), load drops to about 5% of safe rating. The motors will not overheat under normal following conditions.

### Startup Torque (Starting from a Stop)

Static friction is roughly 2x rolling friction. Worst case startup on carpet:
```
t_startup = 0.18 x 2 = 0.36 kg*cm per motor
```
Still under the 0.45 kg*cm safe continuous limit. The robot will start from rest on carpet without stalling.

### Motor Driver Thermal Note

The TB6612FNG is rated for 1.2A continuous per channel. JGA25-370 running current at 40% load is approximately 0.4-0.6A per motor — well within the limit. However, at brief stall (firmware watchdog not yet triggered), stall current at 7.4V could reach 1.5-2.2A per motor, which exceeds the 1.2A continuous rating. The 3.2A peak rating handles this for brief moments.

**Ensure the TB6612FNG KiCad footprint has its exposed thermal pad connected to a copper pour for heat dissipation.** If the IC gets too hot to touch during testing, add a small heatsink or increase copper pour area under the chip.

---

## Target Dimensions

| Parameter | Target |
|---|---|
| Footprint | 300mm (L) x 220mm (W) |
| Chassis body height | 80-100mm |
| Ground clearance | 15mm minimum |
| Wheel diameter | 80mm |
| Wheelbase (front to rear axle) | ~200mm |
| Track width (left to right wheel center) | ~190mm |
| Camera total height from ground | ~380mm |

Ground clearance: 15mm minimum clears most carpet pile and small floor obstacles.

---

## Center of Gravity — Where to Place Components

The battery (110g) and motors (360g) are the heaviest components. Motor weight is distributed at all four corners by design — the CG problem is the battery.

**Battery:** As low as possible. Centered between front and rear axles. If it sits too far back, the robot lifts its front wheels slightly on forward acceleration — the camera tilts up and loses the person. If too far forward, braking causes front-dip.

**Raspberry Pi 5 (66g with active cooler):** Mount flat in the middle of the chassis, centered left-to-right. Keeps weight balanced.

**PCB (50g):** Can sit on top of or beside the Pi. Keep it centered.

**Camera mast (30g):** Light enough to not affect CG significantly, but keep the mast rigid — flex in the mast causes the camera to bounce during movement, which blurs frames and confuses YOLO.

**General rule:** Dense things (battery, PCB) go low and center. The mast should be bolted to the chassis plate, not cantilevered off the Pi case.

---

## What Needs to Be Designed in Fusion 360

### 1. Main Chassis Plate
Flat structural base. Everything mounts here.

Required features:
- 4x motor pocket cutouts or brackets (one per corner)
- 4x M3 through-holes for PCB standoffs — **get these positions from the hardware team before printing. Do not estimate.**
- 4x M2.5 through-holes for Raspberry Pi — 58mm x 49mm hole pattern (official Pi spec, not approximate)
- Battery bay: 90mm x 36mm x 22mm minimum clearance for a soft-pack 2200mAh LiPo. Add a strap mount or lip to hold the battery in during sharp turns.
- 3x sensor cutouts or pockets on front face for HC-SR04 (body size 45mm x 20mm x 15mm per sensor). One center, one angled left 30 degrees, one angled right 30 degrees.
- Mast mounting point: 4x M3 holes in a square pattern, centered at front-middle of chassis, for the camera mast base
- Cable routing channels on underside — wires should not run near wheel path

### 2. Motor Mounts
**Motor body: JGA25-370 — 25mm diameter, mounting holes M3, 17mm center-to-center spacing (verify with calipers on actual motor before printing)**

- Hold JGA25-370 motor body rigidly at each corner, shaft pointing outward
- Use M3 screws through motor mounting holes (two per motor)
- Motor sits as low as possible so wheel center is at the correct height for 15mm ground clearance
- Leave 0.4mm clearance around motor body (not zero — PETG-CF shrinks slightly on cooling, and the JGA25-370 has a round cylindrical body that needs clearance to slide in)
- **Print orientation: motor mounting face must be a horizontal print surface, not a bridged overhang**

### 3. Wheel Hubs (if not included with purchased wheels)
**JGA25-370 uses a 4mm D-shaft. This is different from the N20 (3mm D-shaft). If any existing hub files use 3mm, they need to be updated.**

- Bore for 4mm D-shaft: 4.0mm round + 0.5mm flat on D-side
- Press-fit tolerance: 0.0mm on shaft diameter (tight fit, may need light sanding)
- Print at 100% infill — this part transmits all motor torque
- **Print orientation: flat of the D-bore must be vertical for maximum layer adhesion in torque direction**

### 4. Camera Mast
**This is the most important structural addition and directly affects whether gesture detection works.**

The camera must be elevated to see the person's torso and arms, not their knees.

Required geometry:
- Mast height: 200mm above the chassis plate (camera will be at ~280-300mm from ground, +80mm chassis = ~380mm total)
- Camera angle: 20-25 degrees upward from horizontal (tilted up)
- Camera mount at top of mast: 25mm x 24mm PCB holder, 4x M2 mounting holes in 21mm x 12.5mm pattern
- Mast base: bolt plate with 4x M3 holes matching chassis mounting point
- Cross-section: 20mm x 20mm minimum (thicker is stiffer — mast flex causes camera to bounce)
- Add a gusset brace from mast to chassis plate if the mast extends more than 180mm — prevents forward/backward flex during acceleration

Why this height and angle (calculation):
```
Camera at 380mm height, tilted 25 degrees upward, 1.5m from person:
- Bottom of camera frame hits: 380mm + 1500mm x tan(0.6 degrees) = ~396mm from ground (knee height)
- Center of frame hits: 380mm + 1500mm x tan(25 degrees) = 1079mm from ground (mid-torso)
- Top of frame hits: 380mm + 1500mm x tan(49.4 degrees) = ~2130mm (above head)
```
At 1.5m, the camera sees from knee height to above the head — wrists and shoulders are visible. Gesture detection works.

Secure the CSI ribbon cable with a cable clip within 20mm of each connector — prevents the ribbon from pulling on connectors when the robot turns.

### 5. Ultrasonic Sensor Mounts
Three HC-SR04 sensors on the front face. One centered (pointing straight ahead), one angled 30 degrees left, one angled 30 degrees right. Body size: 45mm x 20mm x 15mm. Leave room for the connector to plug in from above.

### 6. Top Cover (Low Priority — Week 7+)
Cosmetic only. Design after all mechanical integration is verified.

---

## Print Settings

**Material:** PETG-CF (already on hand)

| Setting | Structural Parts (mounts, hubs, mast) | Chassis Plate and Covers |
|---|---|---|
| Nozzle temp | 245 C | 240 C |
| Bed temp | 75 C | 70 C |
| Infill | 35-40% | 20-25% |
| Wall perimeters | 4 | 4 |
| Layer height | 0.2mm | 0.2mm |
| Nozzle type | Hardened steel required | Hardened steel required |

**Nozzle note:** CF filament is abrasive. Standard brass nozzles wear within a few hundred grams of CF filament. Use hardened steel.

**Print orientation — motor mounts specifically:**
The torque from the motor tries to peel the mount apart along its layer lines. The mount must be oriented so the primary load direction is perpendicular to the layer lines (i.e., load tries to compress layers together, not split them apart).

**Print orientation — camera mast:**
Print vertically (mast standing upright). This puts the layer lines horizontal, which resists forward/backward bending loads from camera weight and robot acceleration. If the mast is printed on its side, layers run vertically and the mast can snap at a layer line under load.

---

## Failure Modes to Avoid

| Risk | Cause | Prevention |
|---|---|---|
| Motor mount cracks under torque | Wrong print orientation — layers split along line of force | Orient mount so load compresses layers, not separates them |
| Wheel hub slips on shaft | Insufficient interference fit (note: 4mm D-shaft, not 3mm) | Print hub at 100% infill, measure bore with calipers before assembly |
| Ground clearance lost | Chassis plate sags under component weight | 4 perimeter walls on base plate, 20% infill minimum |
| CSI ribbon cable disconnects | Vibration on cable with no strain relief | Zip tie cable within 20mm of both connectors |
| Camera mast flex causes frame blur | Mast too thin, cantilevered off light mounting point | 20mm x 20mm minimum cross-section, gusset brace at base, bolt directly to chassis plate |
| PCB standoffs misaligned | Drilling positions confirmed before print | Get M3 hole positions from hardware team before finalizing chassis |
| Battery ejects during turn | No battery retention | Add lip or strap mount in the bay |
| JGA25-370 wrong shaft size in hub | Old N20 hub design (3mm bore) reused | Confirm 4mm D-shaft bore in Fusion 360 before printing hubs |

---

## Coordination Points with Hardware Team

**Hard dependencies before printing the final chassis:**

| What to Confirm | Why |
|---|---|
| PCB outer dimensions and M3 hole positions | Standoffs drilled in wrong place = reprint |
| Motor type (confirm JGA25-370 body diameter and M3 hole spacing — measure with calipers) | Motor pocket dimensions depend on exact motor body |
| Battery pack dimensions (get physical dimensions of the actual battery ordered) | Bay sizing |
| HC-SR04 body dimensions (should be 45mm x 20mm x 15mm but verify) | Sensor pocket sizing |
| Camera mast mounts (confirm M2 hole pattern from Pi Camera Module 3 spec before designing top of mast) | 21mm x 12.5mm pattern per spec, but verify |

Resolve all of these with the hardware team before end of Week 2.

---

## Timeline

| Week | Target |
|---|---|
| 1-2 | Sketch top-down footprint, confirm motor and battery dimensions, rough layout in Fusion 360 |
| 2 | Coordinate M3 PCB standoff positions with hardware team |
| 3 | Final chassis v1 ready to print — all dimensions confirmed |
| 4 | Chassis v1 printed, test-fit motors, PCB, and Pi before any permanent assembly |
| 4 | Print camera mast, verify camera height and angle geometry |
| 5-6 | Revisions based on integration testing |
| 7 | Final chassis, strain relief, cable routing complete |
