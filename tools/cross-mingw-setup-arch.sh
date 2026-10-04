#!/usr/bin/env bash
# windows-legacy: install Linux -> Windows (x86_64) cross dependencies.
# Tested on Arch Linux / CachyOS. Adapt package names for other distros.
#
# What you get afterwards:
#   cmake -B cmake-build-cross-mingw -S . \
#     --toolchain cmake/toolchain/windows-mingw64-x86_64.cmake \
#     -DSUNSHINE_ENABLE_WGC=OFF -DSUNSHINE_ENABLE_TRAY=OFF \
#     -DBUILD_TESTS=OFF -DBUILD_DOCS=OFF
#   cmake --build cmake-build-cross-mingw
#   (ZIP packaging: cpack -G ZIP; NSIS/WiX need Windows hosts.)
set -euo pipefail

MINGW_PREFIX="${MINGW_PREFIX:-/usr/x86_64-w64-mingw32}"
JOBS="${JOBS:-$(nproc)}"

msg() { echo "==> $*"; }

# 1. Host tools + bare cross toolchain (pacman).
msg "Installing host packages (needs sudo)..."
sudo pacman -S --needed --noconfirm \
  base-devel cmake ninja mingw-w64-gcc \
  nodejs npm python uv \
  git curl pkgconf

# 2. Cross libraries from AUR (paru). Qt is skipped: legacy builds use TRAY=OFF.
#    WGC is OFF too, so no cppwinrt sysroot package is needed.
msg "Installing AUR cross libraries (this builds Boost, it takes a while)..."
paru -S --needed --noconfirm \
  mingw-w64-pkg-config \
  mingw-w64-zlib \
  mingw-w64-openssl \
  mingw-w64-boost \
  mingw-w64-curl \
  mingw-w64-opus \
  mingw-w64-nlohmann-json

# 3. miniupnpc has no AUR cross package: build it for mingw.
if [ ! -f "$MINGW_PREFIX/lib/pkgconfig/miniupnpc.pc" ]; then
  msg "Building miniupnpc for mingw..."
  rm -rf /tmp/opencode/miniupnp && git clone --depth 1 https://github.com/miniupnp/miniupnp /tmp/opencode/miniupnp
  cmake -S /tmp/opencode/miniupnp/miniupnpc -B /tmp/opencode/miniupnp/build \
    -DCMAKE_SYSTEM_NAME=Windows \
    -DCMAKE_C_COMPILER=x86_64-w64-mingw32-gcc \
    -DCMAKE_CXX_COMPILER=x86_64-w64-mingw32-g++ \
    -DCMAKE_RC_COMPILER=x86_64-w64-mingw32-windres \
    -DCMAKE_INSTALL_PREFIX="$MINGW_PREFIX"
  cmake --build /tmp/opencode/miniupnp/build -j "$JOBS"
  sudo cmake --install /tmp/opencode/miniupnp/build
else
  msg "miniupnpc already in sysroot, skipping."
fi

# 4. Intel VPL dispatcher (provides MFX* symbols for the FFmpeg prebuilt).
#    No AUR cross package exists; build from the intel/libvpl sources.
if [ ! -f "$MINGW_PREFIX/lib/pkgconfig/vpl.pc" ] && [ ! -f "$MINGW_PREFIX/lib/libvpl.dll.a" ]; then
  msg "Building Intel VPL dispatcher for mingw..."
  rm -rf /tmp/opencode/libvpl && git clone --depth 1 https://github.com/intel/libvpl /tmp/opencode/libvpl
  cmake -S /tmp/opencode/libvpl -B /tmp/opencode/libvpl/build \
    -DCMAKE_SYSTEM_NAME=Windows \
    -DCMAKE_C_COMPILER=x86_64-w64-mingw32-gcc \
    -DCMAKE_CXX_COMPILER=x86_64-w64-mingw32-g++ \
    -DCMAKE_RC_COMPILER=x86_64-w64-mingw32-windres \
    -DCMAKE_INSTALL_PREFIX="$MINGW_PREFIX" \
    -DBUILD_EXAMPLES=OFF -DBUILD_TESTS=OFF
  cmake --build /tmp/opencode/libvpl/build -j "$JOBS"
  sudo cmake --install /tmp/opencode/libvpl/build
else
  msg "VPL dispatcher already in sysroot, skipping."
fi

# 5. Submodules (FFmpeg prebuilt tags, enet, web server, common helpers...).
msg "Initializing submodules..."
git submodule update --init --recursive

msg "Done. Configure with:"
echo "  cmake -B cmake-build-cross-mingw -S . --toolchain cmake/toolchain/windows-mingw64-x86_64.cmake -DSUNSHINE_ENABLE_WGC=OFF -DSUNSHINE_ENABLE_TRAY=OFF -DBUILD_TESTS=OFF -DBUILD_DOCS=OFF"
