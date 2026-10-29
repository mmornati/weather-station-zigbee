// Preview scene of the whole station, built from the STL files (run build_stl.sh first).
// Used by render_images.sh for the pictures in the docs and on the website; not printed.
// view = "station" (pole, wind arm, rain post), "head" (box, panel and shield),
// "shield" (radiation shield stack, cut away) or "part" with part_file = "stl/<name>.stl".
// Kit parts (wind arm, anemometer, vane, rain gauge), pole, panel and glands are simple stand-ins.

include <common.scad>

view = "station";
part_file = "stl/pole_saddle.stl";

petg = "#eef1f4";
grey = "#9aa6c2";
dark = "#2b3247";
metal = "#7d8597";
cell_blue = "#1f3f78";
amber = "#ffb547";

pole_h = 3000;
saddle_off = pole_d / 2 + 14;   // pole axis to the saddle's flat face

// Frame on the flat face of a saddle at height h, facing direction a (degrees, 0 = +x):
// x along the face, y up, z away from the pole.
module on_pole(h, a) {
    rotate([0, 0, a]) translate([saddle_off, 0, h])
        multmatrix([[0, 0, 1, 0], [1, 0, 0, 0], [0, 1, 0, 0], [0, 0, 0, 1]]) children();
}

module saddle() { color(petg) rotate([0, 180, 0]) import("stl/pole_saddle.stl"); }
module bracket() { color(petg) import("stl/angle_bracket.stl"); }

// Box with its lid, glands and vent; the saddle square is at the origin.
module box_unit() {
    translate([-68, -68, 0]) {
        color(petg) import("stl/box.stl");
        color(petg) translate([0, 0, 63]) import("stl/box_lid.stl");
        for (i = [0 : 4])
            color(i == 4 ? metal : dark) translate([3 + 14 + i * 24, 0, 33]) rotate([90, 0, 0]) cylinder(d = i == 4 ? 14 : 15, h = 16);
        for (i = [0 : 3])
            color(dark) translate([3 + 14 + i * 24, -16, 33]) rotate([90, 0, 0]) cylinder(d = 5, h = 70);
    }
}

// Panel bracket at 60 degrees, panel facing up and away from the pole.
module panel_unit() {
    w = 136 + 16; h = 110 + 16;
    color(petg) import("stl/panel_cheeks.stl");
    translate([-w / 2, 0, 61]) rotate([-30, 0, 0]) translate([0, -h / 2, 0]) {
        color(petg) import("stl/panel_frame.stl");
        color(cell_blue) translate([8, 8, 2]) cube([136, 110, 3]);
        color("#3d5a94") for (x = [1 : 5]) translate([8 + x * 136 / 6, 8, 5]) cube([0.8, 110, 0.2]);
    }
}

// Radiation shield stack, centre of the base at the origin, z up.
module shield_stack() {
    color(petg) import("stl/shield_base.stl");
    for (i = [0 : 8])
        color(petg) translate([0, 0, 8 + (i + 1) * 17]) rotate([180, 0, 0]) import("stl/shield_plate.stl");
    color(petg) translate([0, 0, 8 + 10 * 17]) rotate([180, 0, 0]) import("stl/shield_cap.stl");
    for (a = [90, 210, 330])
        color(metal) rotate([0, 0, a]) translate([46, 0, 0]) cylinder(d = 5, h = 8 + 10 * 17 + 6);
}

// Shield on an angle bracket: the arm's square sits on the bracket's shelf.
module shield_unit() {
    bracket();
    multmatrix([[0, -1, 0, 0], [0, 0, 1, 36], [-1, 0, 0, 195], [0, 0, 0, 1]]) shield_stack();
}

// Kit rain gauge (stand-in) on the levelling plate, on an angle bracket.
module rain_unit() {
    bracket();
    multmatrix([[1, 0, 0, 0], [0, 0, 1, 36], [0, -1, 0, 33], [0, 0, 0, 1]]) {
        color(petg) import("stl/rain_plate_base.stl");
        color(petg) translate([0, 0, 22]) import("stl/rain_plate_top.stl");
        color(metal) for (p = [[-60, -40], [60, -40], [0, 45]]) translate([p[0], p[1], 5]) cylinder(d = 5, h = 25);
        translate([0, 0, 28]) {
            color("#d9dde3") hull() {
                translate([-60, -35, 0]) cube([120, 70, 6]);
                translate([0, 0, 70]) scale([1, 0.6, 1]) cylinder(d = 150, h = 40);
            }
            color(dark) translate([0, 0, 100]) scale([1, 0.6, 1]) cylinder(d1 = 20, d2 = 140, h = 10.5);
        }
    }
}

// Kit wind arm (stand-in): anemometer on +y, vane on -y.
module wind_arm(z) {
    translate([0, 0, z]) {
        color(grey) translate([-12, -330, -12]) cube([24, 660, 24]);
        color(petg) translate([0, 330, 0]) {
            cylinder(d = 16, h = 90);
            translate([0, 0, 90]) cylinder(d = 40, h = 20);
            for (a = [0, 120, 240]) rotate([0, 0, a]) {
                translate([0, 0, 100]) rotate([0, 90, 0]) cylinder(d = 6, h = 70);
                translate([80, 0, 100]) rotate([90, 0, 0]) difference() {
                    sphere(d = 50);
                    translate([0, 0, -30]) cube(60, center = true);
                    sphere(d = 46);
                }
            }
        }
        color(petg) translate([0, -330, 0]) {
            cylinder(d = 16, h = 80);
            translate([0, 0, 80]) cylinder(d = 30, h = 20);
            translate([-150, -3, 92]) cube([300, 6, 6]);
            translate([-180, -2, 70]) cube([90, 4, 70]);
            translate([150, 0, 95]) rotate([0, 90, 0]) cylinder(d1 = 22, d2 = 0, h = 40);
        }
    }
}

module pole(h0, h1) {
    color(metal) translate([0, 0, h0]) cylinder(d = pole_d, h = h1 - h0);
}

// South is +x: box and panel face south, the shield sits on the north side.
module station_head() {
    on_pole(1200, 0) { saddle(); box_unit(); }
    on_pole(1360, 0) { saddle(); panel_unit(); }
    on_pole(1520, 180) { saddle(); shield_unit(); }
}

if (view == "station") {
    color("#24402c") translate([0, 0, -6]) cylinder(r = 1300, h = 6);
    pole(0, pole_h);
    station_head();
    wind_arm(pole_h - 100);
    translate([-500, -1000, 0]) {
        pole(0, 1000);
        on_pole(860, 0) { saddle(); rain_unit(); }
    }
} else if (view == "head") {
    pole(1040, 1760);
    station_head();
} else if (view == "shield") {
    // cut away a quarter to show the SHT45 cage, the spacers and the overlapping skirts
    difference() {
        shield_stack();
        translate([0, -200, -10]) cube([200, 200, 400]);
    }
    for (a = [90, 210, 330], i = [0 : 9])
        if (a != 330) color(grey) rotate([0, 0, a]) translate([46, 0, 8 + i * 17]) cylinder(d = 10, h = 14);
    color("#3d7a5a") translate([-9, -1, 10]) cube([18, 2, 22]);
} else {
    color(petg) import(part_file);
}
