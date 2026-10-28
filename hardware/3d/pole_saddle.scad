// Pole saddle: a curved block that a stainless worm-drive hose clamp holds against the pole.
// The flat face carries four M4 heat-set inserts on the shared 40 x 40 mm square, where the
// box, the shield arm, the panel bracket and the rain gauge plate bolt on.
// Print flat face down. Four are needed (box, shield, panel, wind arm or rain post).

include <common.scad>

saddle_w = 56;      // across the pole
saddle_h = 60;      // along the pole
saddle_t = 14;      // at the thinnest point, between the pole and the flat face

module pole_saddle() {
    difference() {
        translate([-saddle_w / 2, -saddle_h / 2, 0])
            rounded_box([saddle_w, saddle_h, saddle_t + pole_d / 4], r = 4);
        // Pole groove
        translate([0, 0, saddle_t + pole_d / 2])
            rotate([90, 0, 0]) cylinder(d = pole_d, h = saddle_h + 2, center = true);
        // Two slots for the hose clamps
        for (y = [-saddle_h / 4, saddle_h / 4])
            translate([-saddle_w / 2 - 1, y - clamp_w / 2, saddle_t - clamp_t - 3])
                cube([saddle_w + 2, clamp_w, clamp_t]);
        // Heat-set inserts from the flat face
        for (x = [-1, 1], y = [-1, 1])
            translate([x * mount_pitch / 2, y * mount_pitch / 2, -0.01])
                cylinder(d = m4_insert_d, h = m4_insert_h);
    }
}

pole_saddle();
