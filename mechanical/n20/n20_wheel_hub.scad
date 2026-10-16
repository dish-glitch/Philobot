// Philo bench base — wheel hub for a 75:1 N20 gearmotor (3 mm D-shaft).
// Copies the club's Lego-style hub: two flanges with a groove the tire's
// inner lip snaps into. Measure the blue hub with calipers (in mm) and
// set the values below, then F6 (render) and File > Export > STL.

// ── measure these from the existing blue hub ──
hub_width      = 11.4;  // overall width (measured: 0.45 in)
flange_dia     = 18.0;  // outer diameter of the two flanges  (MEASURE)
groove_dia     = 15.0;  // diameter at the bottom of the middle groove (MEASURE)
groove_width   = 3.0;   // width of the middle groove (MEASURE)

// ── N20 shaft: 3 mm round with one flat. Printed holes come out small,
//    so these are oversized slightly. Loosen/tighten after a test print. ──
shaft_dia      = 3.15;  // round part of the hole
shaft_flat     = 2.55;  // distance from the flat to the far side of the hole
shaft_depth    = 9.0;   // how deep the shaft goes into the hub

$fn = 96;

module d_hole() {
    intersection() {
        cylinder(d = shaft_dia, h = shaft_depth + 0.01);
        // keep everything except a slice on one side → the flat
        translate([-shaft_dia / 2, -shaft_dia / 2, 0])
            cube([shaft_flat, shaft_dia, shaft_depth + 0.01]);
    }
}

module hub() {
    flange_w = (hub_width - groove_width) / 2;
    difference() {
        union() {
            cylinder(d = flange_dia, h = flange_w);                               // inner flange
            translate([0, 0, flange_w]) cylinder(d = groove_dia, h = groove_width); // groove
            translate([0, 0, flange_w + groove_width])
                cylinder(d = flange_dia, h = flange_w);                           // outer flange
        }
        translate([0, 0, -0.01]) d_hole();
    }
}

hub();
