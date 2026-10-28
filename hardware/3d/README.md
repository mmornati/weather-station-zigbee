# 3D-printed parts

OpenSCAD sources for every printed part. Set your dimensions at the top of each file
(`pole_d` in [`common.scad`](common.scad), `panel` in [`panel_bracket.scad`](panel_bracket.scad)),
then render them all:

```bash
hardware/3d/build_stl.sh        # STL files in hardware/3d/stl/
```

Every release also ships a zip of STL files with the default sizes (40 mm pole, 136 × 110 mm panel).
Quantities, orientation and print settings are in the [build guide](../../docs/02-build-guide.md#2-print-the-parts).

| File | Parts |
|---|---|
| [`radiation_shield.scad`](radiation_shield.scad) | 9 plates, cap (window for the LTR390), base with SHT45 cage and arm, spacers |
| [`electronics_box.scad`](electronics_box.scad) | Sealed box (4 × PG7 + vent, gasket groove) and lid |
| [`panel_bracket.scad`](panel_bracket.scad) | Panel frame and tilting cheeks, 45–70° |
| [`pole_saddle.scad`](pole_saddle.scad) | Saddle held by stainless hose clamps; everything bolts to it on a 40 × 40 mm M4 square |
| [`rain_gauge_plate.scad`](rain_gauge_plate.scad) | Levelling plate with bubble pocket for the rain gauge |

These are a first version: print one saddle and the box lid first to check the fit of your
pole and inserts before printing the full set.
