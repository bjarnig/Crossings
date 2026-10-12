#!/usr/bin/env bash
# Configure, build, and install the Crossings SuperCollider plugins.
#
# Usage:
#   ./build.sh [SC_PATH]
#   SC_PATH=/path/to/supercollider ./build.sh
#
# SC_PATH defaults to the 3.14.1 worktree (plugin ABI / sc_api_version v3), which is
# loadable in the installed SuperCollider 3.14.x / 3.12.x apps. Point it at a develop
# checkout only if you run a scsynth built from develop (ABI v6). Installs into the user
# SuperCollider Extensions folder (see CMAKE_INSTALL_PREFIX in CMakeLists.txt).

set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SC_PATH="${1:-${SC_PATH:-/Users/bjarni/Dev/supercollider-3.14.1}}"
BUILD_DIR="${HERE}/build"

echo "==> SuperCollider source: ${SC_PATH}"
echo "==> Build dir:            ${BUILD_DIR}"

cmake -S "${HERE}" -B "${BUILD_DIR}" \
    -DSC_PATH="${SC_PATH}" \
    -DCMAKE_BUILD_TYPE=Release \
    -DSUPERNOVA=OFF

cmake --build "${BUILD_DIR}" --config Release -j

cmake --install "${BUILD_DIR}"

echo
echo "==> Built artifacts:"
find "${BUILD_DIR}" -name '*.scx' -print
echo
echo "==> Installed to: $(cmake -L -N "${BUILD_DIR}" 2>/dev/null | grep '^CMAKE_INSTALL_PREFIX' | cut -d= -f2-)/Crossings"
