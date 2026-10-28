// Rain gauge plate: holds the kit's tipping-bucket gauge level on its own short post.
// Two parts: "base" bolts to a pole saddle (shared 40 x 40 mm square); "top" carries the gauge
// and a bubble level. Three M5 x 30 screws with springs between them set the level.
// Print both flat, PETG, 4 walls, 40 % infill.

include <common.scad>

part = "top";        // "top", "base" or "assembly" (preview only)

plate = [150, 110, 6];
level_d = 15.5;      // round bubble level (15 mm vials are common)
level_depth = 4;
// Slots for the gauge's own screws: two slotted bars cover most kits (adjust if needed).
gauge_slot_y = [-30, 30];
gauge_slot_len = 90;

function level_screws() = [[-60, -40], [60, -40], [0, 45]];

module top() {
    difference() {
        translate([-plate[0] / 2, -plate[1] / 2, 0]) rounded_box(plate, r = 6);
        for (y = gauge_slot_y)
            hull() for (x = [-gauge_slot_len / 2, gauge_slot_len / 2]) translate([x, y, -1]) cylinder(d = m4_clear, h = 10);
        translate([0, 0, plate[2] - level_depth]) cylinder(d = level_d, h = 10);
        for (p = level_screws()) translate([p[0], p[1], -1]) cylinder(d = m5_clear, h = 10);
        // drain holes
        for (x = [-45, 45]) translate([x, 0, -1]) cylinder(d = 8, h = 10);
    }
}

module base() {
    difference() {
        union() {
            translate([-plate[0] / 2, -plate[1] / 2, 0]) rounded_box([plate[0], plate[1], 5], r = 6);
            for (p = level_screws()) translate([p[0], p[1], 0]) cylinder(d = 14, h = 12);
        }
        mount_holes();
        // M5 heat-set inserts (or nuts) for the levelling screws
        for (p = level_screws()) translate([p[0], p[1], 4]) cylinder(d = 6.4, h = 10);
        for (x = [-45, 45]) translate([x, 0, -1]) cylinder(d = 8, h = 10);
    }
}

if (part == "top") top();
else if (part == "base") base();
else {
    base();
    translate([0, 0, 22]) top();
}
