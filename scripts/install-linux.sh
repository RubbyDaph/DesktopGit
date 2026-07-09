#!/usr/bin/env bash
set -euo pipefail

app_name="DesktopGit"
app_id="desktopgit"
script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

appimage_source="${1:-}"
if [[ -z "${appimage_source}" ]]; then
    appimage_source="$(find "${script_dir}" -maxdepth 1 -type f -name "DesktopGit*.AppImage" | head -n 1)"
fi

if [[ -z "${appimage_source}" || ! -f "${appimage_source}" ]]; then
    echo "DesktopGit AppImage was not found."
    echo "Usage: ./install-linux.sh /path/to/DesktopGit-x86_64.AppImage"
    exit 1
fi

icon_source=""
if [[ -f "${script_dir}/desktopgit.svg" ]]; then
    icon_source="${script_dir}/desktopgit.svg"
elif [[ -f "${script_dir}/../packaging/desktopgit.svg" ]]; then
    icon_source="${script_dir}/../packaging/desktopgit.svg"
fi

if [[ -z "${icon_source}" ]]; then
    echo "desktopgit.svg was not found near the installer."
    exit 1
fi

bin_dir="${HOME}/.local/bin"
applications_dir="${HOME}/.local/share/applications"
icons_dir="${HOME}/.local/share/icons/hicolor/scalable/apps"
appimage_target="${bin_dir}/${app_name}.AppImage"
desktop_file="${applications_dir}/${app_id}.desktop"
icon_target="${icons_dir}/${app_id}.svg"

install -d "${bin_dir}" "${applications_dir}" "${icons_dir}"
install -m 755 "${appimage_source}" "${appimage_target}"
install -m 644 "${icon_source}" "${icon_target}"

cat > "${desktop_file}" <<EOF
[Desktop Entry]
Type=Application
Name=${app_name}
Comment=Desktop Git client
Exec=${appimage_target}
Icon=${app_id}
Categories=Development;RevisionControl;
Terminal=false
StartupNotify=true
EOF

chmod +x "${appimage_target}"
chmod 644 "${desktop_file}"

if command -v update-desktop-database >/dev/null 2>&1; then
    update-desktop-database "${applications_dir}" >/dev/null 2>&1 || true
fi

if command -v gtk-update-icon-cache >/dev/null 2>&1; then
    gtk-update-icon-cache "${HOME}/.local/share/icons/hicolor" >/dev/null 2>&1 || true
fi

echo "${app_name} was installed."
echo "AppImage: ${appimage_target}"
echo "Launcher entry: ${desktop_file}"
