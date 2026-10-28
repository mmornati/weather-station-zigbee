// Shared dimensions for the weather station parts. Every part that goes on the pole
// bolts to a pole saddle (pole_saddle.scad) through the same 40 x 40 mm M4 pattern.
// Print in PETG (or ASA): 0.2 mm layers, 4 walls, 30 % gyroid unless a file says otherwise.

$fn = 96;

pole_d = 40;            // outside diameter of your pole, mm (32-40 typical)
mount_pitch = 40;       // M4 bolt square shared by every part
m4_clear = 4.5;         // M4 clearance hole
m4_insert_d = 5.6;      // hole for an M4 x 6 heat-set insert (check your inserts)
m4_insert_h = 6.5;
m5_clear = 5.5;         // M5 rods of the radiation shield
clamp_w = 14;           // worm-drive hose clamp band width + 1 mm
clamp_t = 2.5;          // band thickness + clearance

// Four holes on the shared mounting square, centred on the origin, along Z.
module mount_holes(d = m4_clear, h = 50) {
    for (x = [-1, 1], y = [-1, 1])
        translate([x * mount_pitch / 2, y * mount_pitch / 2, -h / 2])
            cylinder(d = d, h = h);
}

module rounded_box(size, r = 4) {
    hull()
        for (x = [r, size[0] - r], y = [r, size[1] - r])
            translate([x, y, 0]) cylinder(r = r, h = size[2]);
}
