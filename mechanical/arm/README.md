# Robot arm — EEZYbotARM Mk2 (608 bearing mod)

Philo's arm is the [EEZYbotARM Mk2](https://www.myminifactory.com/object/3d-print-eezybotarm-mk2-46620)
by daGHIZmo. Print the other parts from the original release; this folder only holds
the parts changed to use a **608** bearing (8 x 22 x 7 mm, skateboard size) instead of
the **606ZZ** (6 x 17 x 6 mm) the design calls for.

| File | Replaces | Change |
|---|---|---|
| `EBAmk2_013_lower base_608.stl` | `EBAmk2_013_lower base.STL` | Bearing pocket widened 17.3 -> 22.3 mm and deepened 6.0 -> 7.0 mm. Part thickness, holes and ball track unchanged. |
| `EBAmk2_013_lower base_608_7.5mm-balls.stl` | same | 608 pocket **and** ball track re-cut for **7.5 mm** balls (path r = 30 mm, ~22-24 balls). |
| `EBAmk2_013_lower base_608_4.5mm-balls.stl` | same | 608 pocket **and** a raised ring track for **4.5 mm (.177) steel BBs** (path r = 31 mm, ~38-40 balls). |
| `EBAmk2_016_claw gear drive_bigger-horn-pocket.stl` | `EBAmk2_016_claw gear drive.STL` | Looser horn pocket for SG90 single-arm horns that don't fit: hub 7.5 -> 8.4 mm, arm slot ~5.5 -> 6.8 mm wide, pocket 0.6 mm deeper. Print at 100%. |
| `608_to_M6_sleeve.stl` | — (new) | 7.95 mm OD / 6.3 mm ID / 7 mm sleeve so the M6x25 bolt fits the 608's 8 mm bore. |

Ball tracks: the gear above (`011_gearmast`) has a flat underside that rides on the balls
11.85 mm above this plate's bottom. Both alternative tracks put the ball tops at exactly that
height, so the gear, servo-gear mesh and everything above stay where the original design has
them. Fill the track with balls leaving a small gap (don't pack it tight), and add a little grease.

Notes
- Press the 608 in from the bottom; it sits flush, like the 606 did.
- The top lip over the bearing is 1 mm (the original's was 2 mm), so tighten the M6
  lock nut only until the turntable turns freely with no wobble.
- Print both at 100% infill. If the sleeve is loose in the bearing, a drop of glue fixes it.
