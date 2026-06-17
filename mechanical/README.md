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

## Target Dimensions

| Parameter | Target |
|---|---|
| Footprint | ~200mm (L) x 150mm (W) |
| Height (chassis body) | ~60-80mm |
| Ground clearance | ~10mm minimum |
| Wheel diameter | 65mm |
| Wheelbase (front to rear axle) | ~120mm |
| Track width (left to right wheel center) | ~130mm |

These are starting targets — adjust based on motor and wheel availability. Lock in dimensions before the PCB is ordered so mounting holes are in the right place.

---

## Motors

**Type:** N20 micro gear motors with encoders, 12V, 200RPM  
**Shaft:** 3mm D-shaft  
**Mounting:** M2 screws into printed motor bracket

Each motor has a small magnetic encoder on the back (2 signal wires). These plug into the PCB and let the ESP32 measure wheel speed for closed-loop control. The encoder connector position on the PCB depends on where the motors sit — confirm motor placement with the hardware team before the PCB is finalized.

4 motors total. Left-front and left-rear are wired together as one channel. Right-front and right-rear are wired together as the other.

---

## What Needs to Be Designed in Fusion 360

### 1. Main Chassis Plate
Flat base plate. All components mount to this. Needs:
- 4x motor mount pockets (one per corner)
- 4x M3 holes for PCB standoffs (positions locked in with hardware team)
- 4x M2.5 holes for Raspberry Pi standoffs (58mm x 49mm pattern — standard Pi hole spacing)
- Battery bay cutout or cradle (2S LiPo pouch, roughly 70mm x 35mm x 20mm)
- 3x slots or mounts on the front face for ultrasonic sensors (one center, one angled left ~30 degrees, one angled right ~30 degrees)

### 2. Motor Mounts
Press-fit or screw-mount brackets that hold each N20 motor at the corner of the chassis. Motor shaft points outward. Leave room for the wheel to clear the chassis wall.

### 3. Wheel Hubs
If the wheels do not come with a hub that fits the 3mm D-shaft, print one. Tight tolerance — 0.1mm clearance max on the flat side of the D.

### 4. Camera Mount
Front-facing, centered, elevated ~30-40mm above the chassis top. Camera should look slightly downward (~10-15 degree tilt) so it sees the person from feet to head at 1-3 meters distance. Needs to hold the Pi Camera Module v2 (25mm x 24mm board, 4 mounting holes at 2mm).

### 5. Top Cover (optional)
A cosmetic top plate that makes Philo look like a dog body rather than a flat board. Can be designed after everything else works. Low priority until week 7.

---

## Print Settings

Material: PETG-CF (already on hand)

| Setting | Value |
|---|---|
| Nozzle temp | 240-250°C |
| Bed temp | 70-80°C |
| Infill | 25-30% for structural parts |
| Walls | 4 perimeters minimum |
| Layer height | 0.2mm |
| Nozzle | Hardened steel (CF is abrasive) |

Print motor mounts and wheel hubs at 30%+ infill. The chassis plate can be 20% — it is mostly flat and not under high stress.

---

## Coordination Points with Hardware Team

Before printing final chassis:
- Confirm PCB dimensions and M3 standoff hole positions
- Confirm motor type and shaft diameter
- Confirm ultrasonic sensor body size (HC-SR04 is 45mm x 20mm)
- Confirm battery dimensions

These are hard dependencies. A chassis printed before these are confirmed will need reprinting.

---

## Timeline

| Week | Target |
|---|---|
| 1-2 | Sketch top-down layout, confirm motor type, rough footprint in Fusion 360 |
| 3 | Chassis v1 ready to print, dimensions confirmed with hardware team |
| 4 | Chassis v1 printed, test-fit with motors and PCB |
| 5-6 | Revisions based on integration testing |
| 7 | Final chassis, cable management, camera mount finalized |
