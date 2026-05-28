#!/usr/bin/env bash

set -euo pipefail

FIG_DIR="${1:-/Users/miang/Downloads/fig}"
DPI="${2:-300}"
OUTPUT_DIR="${FIG_DIR%/}/png"
TMP_DIR="$(mktemp -d)"

cleanup() {
  rm -rf "${TMP_DIR}"
}

trap cleanup EXIT

if ! command -v gs >/dev/null 2>&1; then
  echo "Error: ghostscript (gs) is required but not installed." >&2
  exit 1
fi

if ! command -v pdfcrop >/dev/null 2>&1; then
  echo "Error: pdfcrop is required for tight export but not installed." >&2
  exit 1
fi

if [[ ! -d "${FIG_DIR}" ]]; then
  echo "Error: directory not found: ${FIG_DIR}" >&2
  exit 1
fi

mkdir -p "${OUTPUT_DIR}"

shopt -s nullglob
pdfs=("${FIG_DIR}"/*.pdf)

if [[ ${#pdfs[@]} -eq 0 ]]; then
  echo "No PDF files found in ${FIG_DIR}"
  exit 0
fi

for pdf in "${pdfs[@]}"; do
  filename="$(basename "${pdf}")"
  stem="${filename%.pdf}"
  cropped_pdf="${TMP_DIR}/${stem}.pdf"

  echo "Cropping ${filename}"
  pdfcrop --margins '0 0 0 0' "${pdf}" "${cropped_pdf}" >/dev/null 2>&1

  pages="$(mdls -name kMDItemNumberOfPages -raw "${cropped_pdf}" 2>/dev/null || echo 1)"

  if [[ -z "${pages}" || "${pages}" == "(null)" ]]; then
    pages=1
  fi

  if [[ "${pages}" == "1" ]]; then
    output="${OUTPUT_DIR}/${stem}.png"
  else
    output="${OUTPUT_DIR}/${stem}-%03d.png"
  fi

  echo "Converting ${filename} -> ${output}"
  gs \
    -dSAFER \
    -dBATCH \
    -dNOPAUSE \
    -sDEVICE=pngalpha \
    -r"${DPI}" \
    -o "${output}" \
    "${cropped_pdf}" >/dev/null
done

echo "Done. PNG files are in ${OUTPUT_DIR}"
