# 3D-printed parts

OpenSCAD sources for every printed part. Set your dimensions at the top of each file
(`pole_d` in [`common.scad`](common.scad), `panel` in [`panel_bracket.scad`](panel_bracket.scad)),
then render them all:

```bash
hardware/3d/build_stl.sh        # STL files in hardware/3d/stl/
```

Every release also ships a zip of STL files with the default sizes (40 mm pole, 136 × 110 mm panel).
Quantities, orientation and print settings are in the [build guide](../../docs/02-build-guide.md#4-print-the-parts).

| File | Parts |
|---|---|
| [`radiation_shield.scad`](radiation_shield.scad) | 9 plates, cap (window for the LTR390), base with SHT45 cage and arm, spacers |
| [`electronics_box.scad`](electronics_box.scad) | Sealed box (4 × PG7 + vent, gasket groove) and lid |
| [`panel_bracket.scad`](panel_bracket.scad) | Panel frame and tilting cheeks, 45–70° |
| [`pole_saddle.scad`](pole_saddle.scad) | Saddle held by stainless hose clamps; everything bolts to it on a 40 × 40 mm M4 square |
| [`angle_bracket.scad`](angle_bracket.scad) | Turns a saddle's vertical face into a shelf, for the shield arm and the rain gauge plate |
| [`rain_gauge_plate.scad`](rain_gauge_plate.scad) | Levelling plate with bubble pocket for the rain gauge |

## Pictures

[`station_render.scad`](station_render.scad) puts the STL files together into the pictures used by the docs and the
website (the wind kit, the rain gauge and the panel are simple stand-ins). After changing a part:

```bash
hardware/3d/build_stl.sh && xvfb-run -a hardware/3d/render_images.sh   # PNG files in site/img/
```

<p align="center"><img src="../../site/img/head.png" alt="Shield, panel and box on the pole" width="260"> <img src="../../site/img/shield.png" alt="Cut-away of the radiation shield" width="320"></p>

These are a first version: print one saddle and the box lid first to check the fit of your
pole and inserts before printing the full set.
