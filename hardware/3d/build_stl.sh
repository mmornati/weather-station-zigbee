#!/usr/bin/env bash
# Renders every printable part to STL (needs OpenSCAD): hardware/3d/build_stl.sh [output dir]
# Used by CI and by the release workflow. Change the parameters at the top of each .scad file
# (pole diameter, panel size...) and run it again.
set -euo pipefail

here="$(cd "$(dirname "$0")" && pwd)"
out="${1:-$here/stl}"
mkdir -p "$out"

parts=(
  "pole_saddle.scad:-:pole_saddle"
  "angle_bracket.scad:-:angle_bracket"
  "radiation_shield.scad:plate:shield_plate"
  "radiation_shield.scad:cap:shield_cap"
  "radiation_shield.scad:base:shield_base"
  "radiation_shield.scad:spacer:shield_spacer"
  "electronics_box.scad:box:box"
  "electronics_box.scad:lid:box_lid"
  "panel_bracket.scad:frame:panel_frame"
  "panel_bracket.scad:cheeks:panel_cheeks"
  "rain_gauge_plate.scad:top:rain_plate_top"
  "rain_gauge_plate.scad:base:rain_plate_base"
)

for entry in "${parts[@]}"; do
  IFS=: read -r file part name <<<"$entry"
  args=()
  [[ "$part" != "-" ]] && args=(-D "part=\"$part\"")
  echo "Rendering $name"
  log="$(openscad "${args[@]}" -o "$out/$name.stl" "$here/$file" 2>&1)"
  if grep -qiE "^(WARNING|ERROR)" <<<"$log"; then
    echo "$log"
    echo "::error::$file ($part) rendered with warnings or errors"
    exit 1
  fi
done
echo "STL files in $out"
