#!/usr/bin/env bash
set -euo pipefail

root_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
tools_dir="${root_dir}/tools"
target="${tools_dir}/linuxdeployqt"
url="https://github.com/probonopd/linuxdeployqt/releases/download/continuous/linuxdeployqt-continuous-x86_64.AppImage"

if ! command -v curl >/dev/null 2>&1; then
    echo "curl was not found. Install curl first."
    exit 1
fi

install -d "${tools_dir}"

echo "Downloading linuxdeployqt..."
curl -L --fail --output "${target}" "${url}"
chmod +x "${target}"

echo "linuxdeployqt was installed to:"
echo "${target}"
echo
echo "Now run:"
echo "./scripts/build-appimage.sh"
