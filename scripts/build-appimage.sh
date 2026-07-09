#!/usr/bin/env bash
set -euo pipefail

root_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
build_dir="${root_dir}/build-release"
appdir="${root_dir}/DesktopGit.AppDir"
linuxdeployqt_path="${LINUXDEPLOYQT:-}"

if [[ -z "${linuxdeployqt_path}" && -x "${root_dir}/tools/linuxdeployqt" ]]; then
    linuxdeployqt_path="${root_dir}/tools/linuxdeployqt"
fi

if [[ -z "${linuxdeployqt_path}" ]] && command -v linuxdeployqt >/dev/null 2>&1; then
    linuxdeployqt_path="$(command -v linuxdeployqt)"
fi

if [[ -z "${linuxdeployqt_path}" ]]; then
    echo "linuxdeployqt was not found in PATH."
    echo "Run ./scripts/install-linuxdeployqt.sh first, then run this script again."
    exit 1
fi

glibc_version="$(ldd --version | head -n 1 | sed -E 's/.* ([0-9]+\.[0-9]+).*/\1/')"
if [[ "${glibc_version}" =~ ^[0-9]+\.[0-9]+$ ]]; then
    newest_version="$(printf "%s\n%s\n" "2.35" "${glibc_version}" | sort -V | tail -n 1)"
    if [[ "${newest_version}" == "${glibc_version}" && "${glibc_version}" != "2.35" ]]; then
        echo "This system uses glibc ${glibc_version}, which is newer than glibc 2.35."
        echo "linuxdeployqt refuses to build portable AppImages on systems newer than Ubuntu 22.04."
        echo "Build the release AppImage on Ubuntu 22.04 or use the GitHub Actions release workflow."
        exit 1
    fi
fi

RunLinuxDeployQt()
{
    APPIMAGE_EXTRACT_AND_RUN=1 \
    "${linuxdeployqt_path}" "$@"
}

cmake -S "${root_dir}" -B "${build_dir}" -DCMAKE_BUILD_TYPE=Release
cmake --build "${build_dir}" --parallel

rm -rf "${appdir}"
RunLinuxDeployQt --version || true

DESTDIR="${appdir}" cmake --install "${build_dir}" --prefix /usr

RunLinuxDeployQt \
    "${appdir}/usr/share/applications/desktopgit.desktop" \
    -appimage \
    -qmldir="${root_dir}/src/ui"

echo "AppImage build finished."
