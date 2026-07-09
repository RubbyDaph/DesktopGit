#!/usr/bin/env bash
set -euo pipefail

app_name="DesktopGit"
app_id="desktopgit"

appimage_target="${HOME}/.local/bin/${app_name}.AppImage"
desktop_file="${HOME}/.local/share/applications/${app_id}.desktop"
icon_target="${HOME}/.local/share/icons/hicolor/scalable/apps/${app_id}.svg"

rm -f "${appimage_target}" "${desktop_file}" "${icon_target}"

if command -v update-desktop-database >/dev/null 2>&1; then
    update-desktop-database "${HOME}/.local/share/applications" >/dev/null 2>&1 || true
fi

if command -v gtk-update-icon-cache >/dev/null 2>&1; then
    gtk-update-icon-cache "${HOME}/.local/share/icons/hicolor" >/dev/null 2>&1 || true
fi

echo "${app_name} was removed from the local launcher."
