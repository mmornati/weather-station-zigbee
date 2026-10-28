// Solar panel bracket: a frame that holds the panel, tilting between 45 and 70 degrees on
// two cheeks bolted to a pole saddle. Mount it just above the electronics box: the panel
// then also shades the box and the battery.
// Print `part` = "frame" flat, "cheeks" flat (one part, two cheeks on a base plate).
// Tilt: M4 pivot bolt + M4 bolt in the arc slot, tighten at about 60 degrees facing south.

include <common.scad>

part = "frame";      // "frame", "cheeks" or "assembly" (preview only)

panel = [136, 110, 3];   // your panel: width, height, thickness (2 W 6 V panels are often 136 x 110)
lip = 3;                 // how much the frame overlaps the panel's edge
frame_w = 8;
frame_t = panel[2] + 4;
cheek_gap = panel[0] + 2 * frame_w + 1;   // the frame sits between the cheeks
cheek_t = 6;
cheek_r = 45;            // pivot to the arc slot

module frame() {
    outer = [panel[0] + 2 * frame_w, panel[1] + 2 * frame_w, frame_t];
    difference() {
        rounded_box(outer, r = 4);
        // pocket for the panel, open on top
        translate([frame_w, frame_w, 2]) cube([panel[0], panel[1], frame_t]);
        // window under the panel (keeps it cool, saves filament)
        translate([frame_w + lip, frame_w + lip, -1]) cube([panel[0] - 2 * lip, panel[1] - 2 * lip, frame_t]);
        // drain notches on the lower edge
        for (x = [0.25, 0.5, 0.75]) translate([outer[0] * x - 5, -1, 2]) cube([10, frame_w + 2, frame_t]);
    }
    // clips that hold the panel in its pocket (snap the panel under them)
    for (x = [0.3, 0.7], y = [0, 1])
        translate([outer[0] * x - 6, y == 0 ? frame_w - lip : outer[1] - frame_w, frame_t - 1.2])
            cube([12, lip, 1.2]);
    // side ears: pivot hole at the middle of the frame, slot bolt hole 27 mm further up
    for (side = [0, 1])
        translate([side == 0 ? -cheek_t / 2 : outer[0] - 0.01, outer[1] / 2 - 8, 0])
            difference() {
                cube([cheek_t / 2 + 0.01, 8 + cheek_r * 0.6 + 8, frame_t]);
                for (y = [8, 8 + cheek_r * 0.6])
                    translate([-1, y, frame_t / 2]) rotate([0, 90, 0]) cylinder(d = m4_clear, h = cheek_t + 2);
            }
}

module cheek() {
    difference() {
        hull() {
            cylinder(r = 10, h = cheek_t);
            translate([-20, -cheek_r - 10, 0]) cube([40, 1, cheek_t]);
        }
        // pivot
        translate([0, 0, -1]) cylinder(d = m4_clear, h = cheek_t + 2);
        // arc slot: 45 to 70 degrees of tilt
        for (a = [45 : 1 : 70])
            hull() {
                rotate([0, 0, -90 + (a - 45)]) translate([cheek_r * 0.6, 0, -1]) cylinder(d = m4_clear, h = cheek_t + 2);
                rotate([0, 0, -90 + (a - 44)]) translate([cheek_r * 0.6, 0, -1]) cylinder(d = m4_clear, h = cheek_t + 2);
            }
    }
}

module cheeks() {
    base = [cheek_gap + 2 * cheek_t, 56, 6];
    difference() {
        translate([-base[0] / 2, -base[1] / 2, 0]) rounded_box(base, r = 4);
        mount_holes();
    }
    // the two cheeks stand on the base plate
    for (x = [-cheek_gap / 2 - cheek_t, cheek_gap / 2])
        translate([x, 0, base[2] + cheek_r + 10]) rotate([90, 0, 90]) cheek();
}

if (part == "frame") frame();
else if (part == "cheeks") cheeks();
else {
    cheeks();
    translate([-(panel[0] + 2 * frame_w) / 2, 0, 6 + cheek_r + 10]) rotate([60, 0, 0]) translate([0, -(panel[1] + 2 * frame_w) / 2, 0]) frame();
}
