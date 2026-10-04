# windows-legacy: Linux -> Windows (x86_64) cross toolchain for MinGW-w64.
#
# Usage (Arch Linux example):
#   sudo pacman -S mingw-w64-gcc
#   # runtime deps via AUR as needed: mingw-w64-boost, openssl, etc.
#   # FFmpeg itself comes from LizardByte/build-deps prebuilts (Windows-AMD64-ffmpeg.tar.gz),
#   # so it does not need to be cross-compiled here.
#   cmake -B cmake-build-cross-mingw -S . \
#     --toolchain cmake/toolchain/windows-mingw64-x86_64.cmake \
#     -DSUNSHINE_ENABLE_WGC=OFF -DSUNSHINE_ENABLE_TRAY=OFF -DBUILD_TESTS=OFF
#   cmake --build cmake-build-cross-mingw
#
# Notes:
# - UCRT vs msvcrt is selected by the mingw-w64-crt build (crt2u.o vs crt2.o).
#   Arch's mingw-w64-crt defaults to UCRT; Win7-SP1-vanilla targets should use
#   an msvcrt sysroot or ensure KB2999226 (UCRT update) on the target.
# - Qt6-static, cppwinrt, oneVPL, MinHook are NOT provided by the bare toolchain.
#   Keep SUNSHINE_ENABLE_TRAY=OFF and SUNSHINE_ENABLE_WGC=OFF until those sysroot
#   packages exist. WGC needs `winrt/windows.graphics.capture.h` + Windowsapp.lib.
# - The frontend (web-ui) uses host-native npm/node; never the Windows npm.cmd path.

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR AMD64)

# Cross compilers (Arch: /usr/bin/x86_64-w64-mingw32-{gcc,g++,windres})
set(CMAKE_C_COMPILER x86_64-w64-mingw32-gcc)
set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++)
set(CMAKE_RC_COMPILER x86_64-w64-mingw32-windres)

# Never run target binaries during cross configure (e.g. try_run).
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# Search target sysroot first, host tools never.
set(CMAKE_FIND_ROOT_PATH /usr/x86_64-w64-mingw32)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# WIN32 is set automatically from CMAKE_SYSTEM_NAME; keep GNU binding checks host-side.
set(CMAKE_CROSSCOMPILING_EMULATOR "")

# windows-legacy: prefer the mingw pkg-config wrapper (AUR mingw-w64-pkg-config)
# so find_package(PkgConfig)/pkg_check_modules resolve cross libs, not host libs.
find_program(MINGW_PKG_CONFIG x86_64-w64-mingw32-pkg-config)
if(MINGW_PKG_CONFIG)
    set(PKG_CONFIG_EXECUTABLE "${MINGW_PKG_CONFIG}" CACHE FILEPATH "pkg-config for MinGW cross")
    set(ENV{PKG_CONFIG_PATH} "/usr/x86_64-w64-mingw32/lib/pkgconfig")
    set(ENV{PKG_CONFIG_LIBDIR} "/usr/x86_64-w64-mingw32/lib/pkgconfig")
endif()

# Node/npm for the web-ui are always host tools (never Windows npm.cmd).
find_program(HOST_NPM npm REQUIRED)
