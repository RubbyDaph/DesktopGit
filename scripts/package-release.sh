#!/usr/bin/env bash
set -euo pipefail

root_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
appimage_source="${1:-}"

if [[ -z "${appimage_source}" ]]; then
    appimage_source="$(find "${root_dir}" -maxdepth 1 -type f -name "DesktopGit*.AppImage" | head -n 1)"
fi

if [[ -z "${appimage_source}" || ! -f "${appimage_source}" ]]; then
    echo "DesktopGit AppImage was not found."
    echo "Usage: ./scripts/package-release.sh /path/to/DesktopGit-x86_64.AppImage"
    exit 1
fi

dist_dir="${root_dir}/dist/DesktopGit-linux-x86_64"
rm -rf "${dist_dir}"
install -d "${dist_dir}"

install -m 755 "${appimage_source}" "${dist_dir}/DesktopGit-x86_64.AppImage"
install -m 755 "${root_dir}/scripts/install-linux.sh" "${dist_dir}/install-linux.sh"
install -m 755 "${root_dir}/scripts/uninstall-linux.sh" "${dist_dir}/uninstall-linux.sh"
install -m 644 "${root_dir}/packaging/desktopgit.svg" "${dist_dir}/desktopgit.svg"

echo "Release files were copied to:"
echo "${dist_dir}"
