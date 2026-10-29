#!/usr/bin/env bash
# Renders the pictures used by the docs and the website (needs OpenSCAD, Python 3 + Pillow,
# and a display: run it under xvfb-run on a headless machine):
#   hardware/3d/build_stl.sh && xvfb-run -a hardware/3d/render_images.sh
# Pictures land in site/img/. Re-run it after changing a part.
set -euo pipefail

here="$(cd "$(dirname "$0")" && pwd)"
out="$here/../../site/img"
tmp="$(mktemp -d)"
trap 'rm -rf "$tmp"' EXIT
mkdir -p "$out"
[[ -f "$here/stl/pole_saddle.stl" ]] || { echo "Run build_stl.sh first"; exit 1; }

shot() {  # name, width, height, camera rotation "rx,ry,rz", extra -D definitions...
  local name="$1" w="$2" h="$3" rot="$4"; shift 4
  echo "Rendering $name"
  openscad -o "$tmp/$name.png" --imgsize="$w,$h" --viewall --autocenter \
    --camera="0,0,0,$rot,0" --colorscheme="Tomorrow Night" "$@" "$here/station_render.scad" >/dev/null 2>&1
}

shot station 1000 1300 "75,0,20" -D 'view="station"'
shot head 1000 1300 "72,0,35" -D 'view="head"'
shot shield 900 900 "65,0,30" -D 'view="shield"'
for p in pole_saddle angle_bracket box box_lid panel_frame panel_cheeks shield_plate shield_cap shield_base shield_spacer rain_plate_top rain_plate_base; do
  shot "part_$p" 800 600 "55,0,30" -D 'view="part"' -D "part_file=\"stl/$p.stl\""
done

# Swap OpenSCAD's background for the site's, crop to the object and add a margin.
python3 - "$tmp" "$out" <<'EOF'
import sys, pathlib
from PIL import Image
src, dst = pathlib.Path(sys.argv[1]), pathlib.Path(sys.argv[2])
bg_in, bg_out = (0x1d, 0x1f, 0x21), (0x0f, 0x15, 0x26)
for f in sorted(src.glob("*.png")):
    im = Image.open(f).convert("RGB")
    px = im.load()
    for y in range(im.height):
        for x in range(im.width):
            if px[x, y] == bg_in:
                px[x, y] = bg_out
    diff = Image.new("L", im.size)
    dp = diff.load()
    for y in range(im.height):
        for x in range(im.width):
            dp[x, y] = 0 if px[x, y] == bg_out else 255
    box = diff.getbbox()
    pad = 40
    im = im.crop((max(box[0] - pad, 0), max(box[1] - pad, 0), min(box[2] + pad, im.width), min(box[3] + pad, im.height)))
    canvas = Image.new("RGB", (im.width, im.height), bg_out)
    canvas.paste(im)
    canvas.save(dst / f.name, optimize=True)
    print("wrote", dst / f.name)
EOF
