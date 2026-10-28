// Radiation shield (Stevenson-style stack) for the SHT45, white PETG or ASA.
// Print `part` = "plate" 9 times, "spacer" 30 times, then "cap" and "base" once each.
// Assembly: three M5 stainless threaded rods through base, spacers and plates, cap on top.
// Each plate is a flat ring with a 45 degree skirt: the skirt of one plate hides the gap
// below it, so neither rain nor direct sun reaches the sensor, while air flows through.
// Plates: 2 walls, 15 % infill, no supports. Base: 4 walls, 30 %.

include <common.scad>

part = "plate";      // "plate", "cap", "base", "spacer" or "assembly" (preview only)

plate_d = 150;       // outside diameter of the skirt
opening_d = 60;      // open centre for airflow
ring_d = 110;        // flat ring, where the rods go
plate_t = 2;         // wall thickness
skirt_drop = (plate_d - ring_d) / 2;  // 45 degree skirt: printable upside down without supports
rod_r = 46;          // radius of the three rods
spacer_h = 14;       // pitch 17 mm: 12 mm of air between two skirts, and the 20 mm skirt
                     // overlaps the plate below, so no straight line of sight reaches the sensor
plate_count = 9;

ltr390_window = true;  // a window in the cap for the optional LTR390 (UV/light)
ptfe_disc_d = 25;      // PTFE disc glued over the window (UV passes, rain doesn't)
ptfe_disc_t = 1;
window_d = 14;

// A flat ring with a 45 degree skirt going down and out. Built with the ring's top face at z = 0
// and the skirt below it, the way it hangs on the pole.
module plate_shell(open = true) {
    difference() {
        union() {
            translate([0, 0, -plate_t]) cylinder(d = ring_d, h = plate_t);
            translate([0, 0, -skirt_drop - plate_t])
                difference() {
                    cylinder(d1 = plate_d, d2 = ring_d, h = skirt_drop + plate_t);
                    translate([0, 0, -0.01])
                        cylinder(d1 = plate_d - 2 * plate_t * 1.414, d2 = ring_d - 2 * plate_t * 1.414, h = skirt_drop + plate_t + 0.02);
                }
            for (a = [90, 210, 330])
                rotate([0, 0, a]) translate([rod_r, 0, -plate_t - 1]) cylinder(d = 14, h = plate_t + 1);
        }
        if (open) translate([0, 0, -10]) cylinder(d = opening_d, h = 20);
        for (a = [90, 210, 330])
            rotate([0, 0, a]) translate([rod_r, 0, -20]) cylinder(d = m5_clear, h = 40);
    }
}

module plate() {
    plate_shell(open = true);
}

module cap() {
    difference() {
        plate_shell(open = false);
        if (ltr390_window) {
            translate([0, 0, -10]) cylinder(d = window_d, h = 20);
            translate([0, 0, -ptfe_disc_t]) cylinder(d = ptfe_disc_d + 0.4, h = 5);
        }
    }
    // Two posts under the window for an LTR390 breakout (Adafruit 4831: M2.5 holes 20.3 mm apart)
    if (ltr390_window)
        for (x = [-10.15, 10.15])
            translate([x, 0, -plate_t - 8])
                difference() {
                    cylinder(d = 5, h = 8);
                    translate([0, 0, -1]) cylinder(d = 2.2, h = 7);
                }
}

module spacer() {
    difference() {
        cylinder(d = 10, h = spacer_h);
        translate([0, 0, -1]) cylinder(d = m5_clear, h = spacer_h + 2);
    }
}

// Base: a solid ring with the rod holes, a cage for the SHT45 in the middle of the stack,
// and an arm that bolts to a pole saddle 120 mm away from the pole.
sht_cage_h = 30;
arm_len = 110;

module base() {
    difference() {
        union() {
            difference() {
                cylinder(d = plate_d, h = 4);
                translate([0, 0, -1]) cylinder(d = opening_d - 10, h = 6);
            }
            for (a = [90, 210, 330])
                rotate([0, 0, a]) translate([rod_r, 0, 0]) cylinder(d = 14, h = 8);
            // perforated tube holding the SHT45 breakout in the middle of the stack
            difference() {
                cylinder(d = 30, h = sht_cage_h);
                translate([0, 0, 2]) cylinder(d = 26, h = sht_cage_h);
                for (z = [8 : 7 : sht_cage_h - 4], a = [0 : 45 : 359])
                    rotate([0, 0, a]) translate([0, 0, z]) rotate([0, 90, 0]) cylinder(d = 4, h = 20);
            }
            // spokes to the cage
            for (a = [30, 150, 270])
                rotate([0, 0, a]) translate([0, -2, 0]) cube([opening_d / 2, 4, 4]);
            // arm towards the pole (+x), ending on the shared 40 x 40 mounting square
            translate([plate_d / 2 - 10, -26, 0]) cube([arm_len, 52, 6]);
            translate([plate_d / 2 + arm_len - 46, -26, 0]) cube([46, 52, 10]);
        }
        for (a = [90, 210, 330])
            rotate([0, 0, a]) translate([rod_r, 0, -1]) cylinder(d = m5_clear, h = 20);
        // cable hole for the sensor
        translate([0, 0, -1]) cylinder(d = 6, h = 5);
        translate([plate_d / 2 + arm_len - 23, 0, 0]) mount_holes();
    }
}

module assembly() {
    color("lightgray") base();
    for (i = [0 : plate_count - 1])
        translate([0, 0, 8 + (i + 1) * (spacer_h + plate_t + 1)]) plate();
    translate([0, 0, 8 + (plate_count + 1) * (spacer_h + plate_t + 1)]) cap();
}

// Plates and cap print upside down: flat ring on the bed, skirt rising at 45 degrees.
if (part == "plate") rotate([180, 0, 0]) plate();
else if (part == "cap") rotate([180, 0, 0]) cap();
else if (part == "base") base();
else if (part == "spacer") spacer();
else assembly();
