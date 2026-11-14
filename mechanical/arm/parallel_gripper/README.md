# Parallel gripper for the EEZYbotArm Mk2, M3 version

Based on sthone's "Parallel Gripper for EEZYbotARM MK2", a remix of their
"ROV Manipulator (Parallel Gripper)". These are the original parts with the
imperial screw holes resized for **M3 × 20** screws. Original design and
credit: sthone, on Thingiverse/Printables.

| File | Print | Change |
|---|---|---|
| Robotic_Arm_Finger_M3.stl | 2 | 2-56 holes (2.03 mm) → 2.6 mm, so an M3 cuts its own thread |
| Robotic_Arm_Forward_Hinge_Arm_M3.stl | 2 | Both pivot holes → 3.3 mm (M3 turns freely) |
| Robotic_Arm_Gear_Arm_Right_M3.stl | 1 | Both pivot holes → 3.3 mm |
| Robotic_Arm_Servo_Gear.stl | 1 | None. Original 7.4 mm horn-hub pocket. |
| Robotic_Arm_Servo_Gear_7.7mm-hub.stl | (alt, recommended) | Hub pocket 7.4 -> 7.7 mm. The MG90S horn fit the original Mk2 claw gear (7.5 mm pocket); this allows for prints coming out slightly undersize. Print only one of the two. |
| Robotic_Arm_Finger_Pad.stl | 2 | None. Print in TPU, or use PLA with rubber stuck on. |
| Robotic_Arm_Gear_Arm_Left_M3.stl | 1 | Both pivot holes -> 3.3 mm (same as the right arm) |
| Robotic_Arm_Palm_M3.stl | 1 | 4 pivot holes -> 2.6 mm M3 pilot, 1 mm deeper |
| Robotic_Arm_Palm_Top_M3.stl | 1 | All 4 pivot holes are 3.3 mm clearance with a 6 mm counterbore, so an M3x20 head sits 3.9 mm down and the tip lands about 4 mm into the Palm floor (checked against the Palm_Top resting on the Palm walls) |
| Hinge_Pivot_Spacer_M3.stl | 2 | New. A 4.7 OD x 4.9 mm tube that keeps each forward hinge arm from riding up the screw |


## Screws (M3 × 20, 8 total)
- **Finger joints, 4 screws.** Each finger has a 3.2 mm tab, a 6.3 mm slot
  for the arm, and another 3.2 mm tab (12.7 mm total). The screw goes through
  the top tab and the arm, then threads into the bottom tab. A 20 mm screw
  sticks out about 7 mm underneath. Add an M3 nut there to lock it, or cut
  the screw down. Don't tighten so hard that the arm can't swing.
- **Palm pivots, 4 screws.** All 4 go in from the **top of Palm_Top**, down
  through Palm_Top, then the arm, then thread into the Palm floor. This is
  different from the original, which used 1" flat heads for the gear arms and
  short button heads under the top for the hinge arms. Put a spacer on each
  hinge-arm screw, between the hinge arm and Palm_Top. Tighten until snug,
  then check the fingers still open and close freely.
