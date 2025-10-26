#!/bin/bash
# install_inputs.sh
# Script seeded by AI (GPT-5) and manually refined.
# This script downloads two sparse matrix tarballs and extracts them for use with gs_patterns

set -euo pipefail
trap 'echo "Error on line $LINENO"; exit 1' ERR

BASE_URL="https://suitesparse-collection-website.herokuapp.com/MM/HB"
DEST_DIR="datasets"
MATRICES=( "bcsstk08" "bcsstm01" )

mkdir -p "$DEST_DIR"

for name in "${MATRICES[@]}"; do
  url="${BASE_URL}/${name}.tar.gz"
  out="${DEST_DIR}/${name}.tar.gz"

  echo "Downloading ${name} ..."
  wget -q --show-progress -L -O "$out" "$url"

  echo "Extracting ${out} ..."
  tar -xzf "$out" -C "$DEST_DIR"
  rm -f "$out"
  echo "[CLEAN] ${name}: removed ${out}"

done

cat <<'EOT'

Reminder - If you use these matrices in your own work, please cite the following paper:
Kolodziej et al. (2019). "The SuiteSparse Matrix Collection Website Interface."
Journal of Open Source Software, 4(35), 1244. DOI: https://doi.org/10.21105/joss.01244

EOT