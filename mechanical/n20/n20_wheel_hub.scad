// Philo bench base — rim for a 75:1 N20 gearmotor (3 mm D-shaft) that takes a
// LEGO tire 89201 / 24341 ("Tire Ø24 x 14 shallow tread"): 18 mm inner dia,
// 24 mm outer dia, 14 mm wide. Same size as LEGO rim 55982 (Ø18 x 14).
// A ready-made STL with these values is next to this file (n20_rim_lego_89201.stl).
// To tweak: change a value, F6 (render), File > Export > STL.

// ── tire fit ──
rim_width    = 14.0;   // tire width
seat_dia     = 18.3;   // where the tire sits: tire ID is 18 mm, +0.3 so it grips
lip_dia      = 19.6;   // raised lips at both edges keep the tire from walking off
lip_width    = 1.2;    // width of each lip
groove_width = 3.0;    // centre groove for the ridge inside the tire
groove_dia   = 16.3;   // groove bottom (≈1 mm deep). Tire too tight → lower it

// ── N20 shaft: 3 mm round with one flat. Printed holes come out small, so these
//    are slightly oversized. Loosen/tighten after one test print. ──
shaft_dia    = 3.15;   // round part of the hole
shaft_flat   = 2.55;   // flat to the far side of the hole
shaft_depth  = 9.0;    // how deep the shaft goes in (N20 shafts are ~9-10 mm)

$fn = 96;

module d_hole() {
    intersection() {
        cylinder(d = shaft_dia, h = shaft_depth + 0.01);
        translate([-shaft_dia / 2, -shaft_dia / 2, 0])
            cube([shaft_flat, shaft_dia, shaft_depth + 0.01]);
    }
}

module rim() {
    difference() {
        union() {
            cylinder(d = lip_dia, h = lip_width);                                      // inner lip
            cylinder(d = seat_dia, h = rim_width);                                     // tire seat
            translate([0, 0, rim_width - lip_width]) cylinder(d = lip_dia, h = lip_width); // outer lip
        }
        // centre groove the tire's inner ridge snaps into
        translate([0, 0, (rim_width - groove_width) / 2])
            difference() {
                cylinder(d = seat_dia + 1, h = groove_width);
                translate([0, 0, -0.01]) cylinder(d = groove_dia, h = groove_width + 0.02);
            }
        translate([0, 0, -0.01]) d_hole();
    }
}

rim();
