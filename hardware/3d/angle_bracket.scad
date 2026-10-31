// Angle bracket: turns the vertical face of a pole saddle into a horizontal shelf.
// The radiation shield arm and the rain gauge plate have their 40 x 40 mm M4 square on a
// horizontal face, so each of them sits on one of these brackets.
// Vertical leg: 4 x M4 x 12 into the saddle's inserts. Shelf: 4 x M4 x 20 + nyloc nuts.
// Print on its side (one leg flat on the bed, the gussets standing), PETG, 6 walls, 50 % infill.

include <common.scad>

leg = 60;            // width of both legs, and depth of the shelf
leg_t = 6;
gusset_t = 5;

module angle_bracket() {
    difference() {
        union() {
            // vertical leg against the saddle (x-y plane, z = 0, y up)
            translate([-leg / 2, -leg / 2, 0]) rounded_box([leg, leg + leg_t, leg_t], r = 4);
            // shelf on top of it, sticking out along +z
            translate([-leg / 2, leg / 2, 0]) cube([leg, leg_t, leg]);
            // gussets under the outer edges of the shelf, clear of the screw heads
            for (x = [-leg / 2, leg / 2 - gusset_t])
                hull() {
                    translate([x, leg / 2 - 1, leg_t]) cube([gusset_t, 1, leg - leg_t - 4]);
                    translate([x, -leg / 2 + 12, leg_t]) cube([gusset_t, leg - 12, 1]);
                }
        }
        // holes to the saddle
        mount_holes();
        // holes in the shelf, square centred on the part of the shelf beyond the leg
        translate([0, leg / 2, leg / 2 + leg_t / 2]) rotate([90, 0, 0]) mount_holes();
    }
}

angle_bracket();
