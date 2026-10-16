#!/bin/sh
# Compile the MML songs to VGM (input for SGDK's XGM2 converter). Needs ctrmml's mmlc in PATH or MMLC set.
set -e
cd "$(dirname "$0")"
MMLC=${MMLC:-mmlc}
python3 compose.py >/dev/null
for f in mml/*.mml; do
  n=$(basename "$f" .mml)
  "$MMLC" -o "../../res/snd/$n.vgm" "$f" >/dev/null
done
echo "VGM written to res/snd"
