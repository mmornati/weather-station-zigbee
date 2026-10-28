// Electronics box: XIAO ESP32-C6 on a perfboard, bq24074 charger, 18650 holder, BME280.
// Hangs on a pole saddle with the cable entries facing DOWN, so water never runs into a gland.
// Print `part` = "box" on its back (no supports) and "lid" flat. PETG, 4 walls, 30 % infill.
// Seal: 2 mm silicone cord in the groove, lid closed with 4 x M4 x 12 into heat-set inserts.
// Bottom face: 4 x PG7 cable glands (wind, rain, shield sensor, solar panel) + 1 x M12 vent plug.

include <common.scad>

part = "box";        // "box", "lid" or "assembly" (preview only)

inner = [130, 60, 100];   // width (x), depth (y), height (z, vertical when mounted)
wall = 3;
corner_r = 6;
cord_d = 2;               // silicone cord for the seal
groove_w = cord_d + 0.3;
groove_depth = cord_d * 0.7;   // the lid squeezes the cord by about 30 %
gland_d = 12.5;           // PG7 thread
vent_d = 12.2;            // M12 x 1.5 vent plug
lid_t = 4;

outer = [inner[0] + 2 * wall, inner[1] + wall, inner[2] + 2 * wall];
boss_d = 11;

// Body: built lying on its back (the face against the pole is z = 0), open towards +z.
// x = width, y = height (y = 0 is the bottom face with the glands), z = depth.
module box() {
    difference() {
        union() {
            rounded_box([outer[0], outer[2], outer[1]], r = corner_r);
        }
        // cavity
        translate([wall, wall, wall]) rounded_box([inner[0], inner[2], inner[1] + 1], r = corner_r - wall);
        // seal groove on the rim
        translate([wall / 2 - groove_w / 2 + 0.5, wall / 2 - groove_w / 2 + 0.5, outer[1] - groove_depth])
            difference() {
                rounded_box([outer[0] - wall + groove_w - 1, outer[2] - wall + groove_w - 1, groove_depth + 1], r = corner_r);
                translate([groove_w, groove_w, -1])
                    rounded_box([outer[0] - wall - groove_w - 1, outer[2] - wall - groove_w - 1, groove_depth + 3], r = corner_r - 1);
            }
        // glands and vent through the bottom wall (y = 0), centred in the depth
        for (i = [0 : 3])
            translate([wall + 14 + i * 24, -1, wall + inner[1] / 2]) rotate([-90, 0, 0]) cylinder(d = gland_d, h = wall + 2);
        translate([wall + 14 + 4 * 24, -1, wall + inner[1] / 2]) rotate([-90, 0, 0]) cylinder(d = vent_d, h = wall + 2);
        // pole mount: the shared 40 x 40 square, M4 from the inside into the saddle's inserts
        translate([outer[0] / 2, outer[2] / 2 + 15, 0]) mount_holes();
        // lid inserts in the corner bosses
        for (p = lid_screws()) translate([p[0], p[1], outer[1] - m4_insert_h]) cylinder(d = m4_insert_d, h = m4_insert_h + 1);
    }
    // corner bosses for the lid screws
    for (p = lid_screws())
        translate([p[0], p[1], 0])
            difference() {
                cylinder(d = boss_d, h = outer[1] - groove_depth - 0.5);
                translate([0, 0, outer[1] - m4_insert_h - groove_depth]) cylinder(d = m4_insert_d, h = 20);
            }
    // M3 insert posts for a 70 x 50 mm perfboard (holes 64 x 44 mm apart)
    for (x = [-32, 32], y = [-22, 22])
        translate([outer[0] / 2 + x, outer[2] / 2 + 15 + y, wall])
            difference() {
                cylinder(d = 7, h = 8);
                cylinder(d = 4.2, h = 9);
            }
    // cradle for the 18650 holder, along the top
    translate([outer[0] / 2 - 40, outer[2] - wall - 28, wall])
        difference() {
            cube([80, 24, 5]);
            translate([3, 3, 1]) cube([74, 18, 5]);
        }
}

function lid_screws() = [
    [wall + boss_d / 2 - 1, wall + boss_d / 2 - 1],
    [outer[0] - wall - boss_d / 2 + 1, wall + boss_d / 2 - 1],
    [wall + boss_d / 2 - 1, outer[2] - wall - boss_d / 2 + 1],
    [outer[0] - wall - boss_d / 2 + 1, outer[2] - wall - boss_d / 2 + 1],
];

module lid() {
    difference() {
        rounded_box([outer[0], outer[2], lid_t], r = corner_r);
        for (p = lid_screws()) translate([p[0], p[1], -1]) cylinder(d = m4_clear, h = lid_t + 2);
    }
    // a small drip edge on the top side (y = max) so rain runs off the joint
    translate([0, outer[2] - 2, lid_t]) cube([outer[0], 2, 3]);
}

if (part == "box") box();
else if (part == "lid") lid();
else {
    box();
    translate([0, 0, outer[1] + 10]) color("white", 0.6) lid();
}
