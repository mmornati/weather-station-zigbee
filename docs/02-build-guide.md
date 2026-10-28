# 02 · Step-by-step build

## 1. Bench test first

1. Flash the firmware ([how](03-firmware.md#build-and-flash)) with the XIAO on USB.
2. Wire the kit, the SHT45 and the BME280 on a breadboard, without the charger.
3. Open the serial monitor and press **RESET**. You should see the modules detected, the station joining Zigbee2MQTT and a first report.
4. Spin the cups and tip the bucket during the 2 minutes the station stays awake after a reset: the next report shows the wind and the rain.

## 2. Print the parts

All parts are OpenSCAD files in [`hardware/3d`](../hardware/3d). Open one, adjust the parameters at the top (pole diameter in `common.scad`, panel size in `panel_bracket.scad`), and export to STL, or run `hardware/3d/build_stl.sh` to render all of them. Every release also ships a zip of STL files.

| Part | File (`part=`) | Qty | Orientation | Settings |
|---|---|---|---|---|
| Shield plate | `radiation_shield.scad` (`plate`) | 9 | As exported (flat ring down, skirt up at 45°) | White, 2 walls, 15 % |
| Shield cap | `radiation_shield.scad` (`cap`) | 1 | As exported | White, 2 walls, 15 % |
| Shield base + arm | `radiation_shield.scad` (`base`) | 1 | Flat | 4 walls, 30 % |
| Shield spacer | `radiation_shield.scad` (`spacer`) | 30 | Upright | 100 % |
| Electronics box | `electronics_box.scad` (`box`) | 1 | On its back | 4 walls, 30 % |
| Box lid | `electronics_box.scad` (`lid`) | 1 | Flat | 4 walls, 30 % |
| Panel frame | `panel_bracket.scad` (`frame`) | 1 | Flat | 4 walls, 30 % |
| Panel cheeks | `panel_bracket.scad` (`cheeks`) | 1 | Base plate down | 4 walls, 40 % |
| Pole saddle | `pole_saddle.scad` | 4 | Flat face down | 6 walls, 40 % |
| Rain gauge plate | `rain_gauge_plate.scad` (`top`, `base`) | 1 + 1 | Flat | 4 walls, 40 % |

Material: **PETG** (235–245 °C, bed 75–80 °C) in a light colour, or ASA if your hotend handles ~250 °C (the stock Ender 3 V2 Neo hotend is PTFE-lined). Not PLA: it softens in summer sun. Every part fits a 220 × 220 mm bed.

Heat-set inserts: M4 × 6 in the saddles and the box corners, M3 in the box's perfboard posts, M5 (or nuts) in the rain plate base.

## 3. Build the box

1. Solder the XIAO, the resistors and capacitors, and the two RJ11 jacks on a 70 × 50 mm perfboard; screw it on the four M3 posts.
2. Fit the 18650 holder in the cradle at the top, the bq24074 and the BME280 next to it. Tape the NTC to the cell.
3. Fit four PG7 glands and the vent plug in the bottom face. Cables: wind (RJ11 #1), rain (RJ11 #2), shield sensors, solar panel.
4. Press the 2 mm silicone cord into the groove and close the lid with 4 × M4 × 12.

## 4. Assemble the shield

Push the three M5 rods through the base, then alternate spacer and plate nine times, and finish with the cap. The SHT45 sits in the perforated cage of the base, in the middle of the stack. The optional LTR390 screws under the cap's window, with the PTFE disc glued in the recess on top.

## 5. Install

- **One 3 m pole** in the open garden, at least 3 m from walls and paving.
- Wind arm (from the kit) at the top. Point the vane's **N** mark north (a phone compass is fine).
- Radiation shield at **1.5 m** on the north side of the pole, on its arm.
- Box at ~1.2 m, glands down; panel just above it facing **south**, tilted about **60°**.
- **Rain gauge** on its own short post about 1 m away, levelled with the bubble.
- Optional probes: one DS18B20 5 cm above the grass, out of the shield; the other 10 cm deep in the soil.

Next: [Firmware](03-firmware.md).
