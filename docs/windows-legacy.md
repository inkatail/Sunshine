/**
 * @file docs/windows-legacy.md
 * @brief Windows legacy branch status (windows-legacy): native 7 / 8.x support.
 */

# Windows legacy branch (`windows-legacy`)

Goal: make Sunshine run **natively** on Windows 7 SP1 (and 8.x) without
SecondSystem / VxKex, from the `inkatail/Sunshine` fork.

Branch: `windows-legacy` (this branch).

## What works / what is gated in this iteration

### Capture backends

| Backend | API floor | Status on this branch |
|---------|-----------|----------------------|
| Desktop Duplication (DDX) | Windows 8+ (`IDXGIOutput1::DuplicateOutput`) | **Enabled on 8.x/10+, disabled on 7/Vista.** `duplication_t::init()` and `test_dxgi_duplication()` fail fast pre-8 via `os_version.h` (`RtlGetVersion`). `platf::display()` skips DDX when `!is_win8_or_greater()`. `DuplicateOutput1` (10+) falls back to `DuplicateOutput` (8+) via `QueryInterface`. |
| NVIDIA Framebuffer Capture (NvFBC) | NVIDIA driver, Vista+ | **Implemented (ToSys target).** `src/platform/windows/nvfbc_win.h` (runtime ABI, no SDK needed at build), `nvfbc.cpp` (loader + session + RAM backend), `display_nvfbc_ram_t` (software encode) and `display_nvfbc_vram_t` (D3D11 upload for NVENC) in `display_vram.cpp`. Selected with `capture=nvfbc`, or automatically after DDX and before WGC. This is the GameStream-era path for Windows 7. |
| Windows.Graphics.Capture (WGC) | Windows 10+ (1803 for monitors) | **Enabled on 10+ only, optional at compile time.** `wgc_capture_t::init()` fails fast pre-10. `platf::display()` skips WGC pre-10. Build with `-DSUNSHINE_ENABLE_WGC=OFF` to exclude `display_wgc.cpp`, `winrt/windows.graphics.capture.h`, and `Windowsapp.lib` for Win7/8.x builds. |
| GDI / DWM fallback | Vista+ (`BitBlt`, `GetDC`, DWM) | **Not implemented (deferred per branch decision).** AMD/Intel on Win7 stay unsupported for now with a clear log. |

Net effect today:

- **Windows 7 + NVIDIA:** NvFBC ToSys captures (driver DLL loaded at runtime), software encode via `display_nvfbc_ram_t`, NVENC via `display_nvfbc_vram_t` (D3D11 upload). No DDX/WGC. This mirrors GameStream, which used NvFBC + NVENC on Win7.
- **Windows 7 + AMD/Intel:** no capture backend yet (GDI deferred); clear log, no crash.
- **Windows 8.0/8.1:** DDX stays enabled (`DuplicateOutput`, not `DuplicateOutput1`). WGC is skipped. Win10-only code paths (Output6 HDR, `win32u.dll` hook, `SetThreadDescription`, `SetProcessDpiAwarenessContext`, high-resolution timer flag, `JOB_LIST`) are guarded. Covered by `tests/unit/test_os_version.cpp` (`6.2`/`6.3` → DDX true, WGC false).
- **Windows 10/11:** unchanged behavior (DDX first, NvFBC available on demand, WGC fallback). Covered by `10.0` → DDX true, WGC true.

### NvFBC on Windows (implemented)

NvFBC is capture, NVENC is encode; GameStream used both. The pre-existing
NvFBC code (`src/platform/linux/cuda.cpp`, `third-party/nvfbc/NvFBC.h`) is the
Linux X11/CUDA variant and cannot be reused on Windows, so this branch adds a
Windows implementation:

- `src/platform/windows/nvfbc_win.h`: runtime ABI for `NvFBC64.dll` /
  `NvFBC.dll` (entry points, ToSys structs/interface, version packing).
  Struct layouts follow the Capture SDK binary interface; `static_assert`s on
  sizes guard against drift. No SDK or link-time dependency.
- `src/platform/windows/nvfbc.cpp`: process-lifetime loader (explicit
  System32 path), one-shot `NvFBC_Enable`, `GetStatusEx` probe, `CreateEx`
  ToSys session with HW-cursor compositing, `WAIT_WITH_TIMEOUT` grab loop
  with `reinit` on invalidation and throttled DRM warnings, plus the
  `display_nvfbc_ram_t` backend (GDI geometry, DXGI factory only for the
  shared `IsCurrent()` loop, ARGB crop into software images).
- `display_nvfbc_vram_t` in `display_vram.cpp`: same capture, uploaded via a
  D3D11 staging texture into the shared encoder texture for NVENC.
- Optimizations ported from the DXGI backends: diff-map damage detection
  (`bDiffMap`, default 128x128 blocks, graceful fallback when refused) skips
  the copy and encode on static screens exactly like DXGI's
  `AccumulatedFrames` check; `syncThreadDesktop()` in both inits so service
  (session 0) launches capture the user desktop; adapter-ordinal retry
  against the default adapter; throttled DRM warnings.
- Known limitations (not defects, documented): the HW cursor is composited by
  the driver, so it is always visible even when the client hides it
  (GameStream behaved the same); multi-monitor crop assumes the NvFBC buffer
  origin is the virtual-desktop origin (exact for single-display hosts);
  SDR only (no `bHDRRequest`); ToSys round-trips through system memory — the
  zero-copy Dx9Vid target is future work.
- Factory order in `display_base.cpp`: explicit `capture=nvfbc` anywhere;
  autodetect tries DDX, then NvFBC, then WGC. New `nvfbc` option in the
  Advanced tab (Windows). `display_names()` falls back to GDI enumeration
  when NvFBC is available but duplication probing fails (Win7).
- Can NvFBC be used on Windows? Yes: the driver exports `NvFBC_CreateEx` /
  `GetStatusEx` / `Enable` from `%WINDIR%\System32\NvFBC64.dll`
  (`SysWOW64\NvFBC.dll` for 32-bit), one session per head, session created at
  startup before the game goes fullscreen. Officially licensed for
  Quadro/Tesla/GRID; on GeForce, `CreateEx` needs 16 bytes of private data.
  Sourcing, in order: `NVFBC_PRIV_DATA` hex env (LookingGlass convention),
  built-in public magic, or the nvfbcwrp wrapper DLL (renames the system DLLs
  and injects the key). See `nvfbc.cpp` (`priv_data_key`) and the ABI header
  references.

### NvFBC vs NVENC (no encoder adjustment needed)

- **NvFBC = capture.** Linux-only code (`src/platform/linux/cuda.cpp:13,725,888,1115`) plus the new Windows ToSys backend above (`src/platform/windows/nvfbc.cpp`). Nothing else to adjust.
- **NVENC = encode, already Windows-native.** `src/nvenc/nvenc_d3d11_native.cpp:15` (`NV_ENC_DEVICE_TYPE_DIRECTX` + `NvEncRegisterResource`), `nvenc_d3d11_on_cuda.cpp:15`, `nvenc_dynamic_factory.cpp:20,82` (`nvEncodeAPI{64,a64,}.dll`, SDK 11.0–13.1, min driver `456.71`). It consumes D3D11 textures from whatever capture backend is active (`display_vram.cpp:2133`), so NvFBC VRAM upload feeds it directly on Win7. No encoder change in this iteration.

### Loader / API guards in this iteration

- `src/platform/windows/os_version.h` (new): `RtlGetVersion`-based helpers (`is_win8_or_greater`, `is_win10_or_greater`, `is_windows_7`). No manifest dependency. Pure predicates in `os_version_check.h`, covered by `tests/unit/test_os_version.cpp`.
- `display_base.cpp`: early-out in `duplication_t::init` + `test_dxgi_duplication`; `win32u.dll` hook only on 10+ with `MH_CreateHookApi` result checked; factory logs when backends are disabled; 8.1 `shcore.dll` DPI-awareness fallback before `SetProcessDPIAware`.
- `display.h`/`display_base.cpp`: `d3d11_create_device_retry()` drops `11_1` (vanilla Win7 without KB2670838) and `VIDEO_SUPPORT` (pre-8) on `E_INVALIDARG`; used by the duplication probe, the capture device, the encoder device, and the NvFBC upload device.
- `display_vram.cpp`: shared textures use NT handles on 8+ and legacy `SHARED` + `IDXGIResource::GetSharedHandle` on 7; encoder opens via `OpenSharedResource1` on 8+ and `OpenSharedResource` on 7.
- `display_wgc.cpp`: early-out pre-10 before any WinRT activation.
- `display.h` + CMake: `SUNSHINE_ENABLE_WGC` (default `ON`). When `OFF`, defines `SUNSHINE_NO_WGC`, excludes `display_wgc.cpp`, omits `Windowsapp.lib` (UWP umbrella, Win10+).
- `misc.cpp`: `SetThreadDescription` via `GetProcAddress` (no-op on 7/8.x); `PROC_THREAD_ATTRIBUTE_JOB_LIST` failure falls back to no job tracking instead of breaking launch; `CREATE_WAITABLE_TIMER_HIGH_RESOLUTION` has an SDK fallback define + runtime fallback.
- `tools/dxgi.cpp`: `WINVER/_WIN32_WINNT=0x0601`, DPI-awareness-context via `GetProcAddress`, then 8.1 `shcore` awareness, then `SetProcessDPIAware` fallback.
- `tools/sunshinesvc.cpp`: `JOB_LIST` assignment failure runs Sunshine without job tracking instead of failing on Win7.
- `input.cpp`: `WINVER/_WIN32_WINNT=0x0601` (no Win10-only static imports in this TU).
- Include case: `<Windows.h>`/`<WinUser.h>` lowercased to `<windows.h>`/`<winuser.h>` repo-wide so Linux cross compilers on case-sensitive filesystems resolve them.

`NTDDI_VERSION=NTDDI_WIN10` in `misc.cpp` is intentionally left as-is: it keeps
`IDXGIOutput5/6` declarations visible for 8.x/10+ builds. Downlevel execution is
handled by `QueryInterface` / `GetProcAddress` guards, not by lowering the SDK.

## Known remaining Win7 blockers (not fixed here)

These must be addressed before a Win7 binary is usable. Listed so the next
commits have a checklist:

1. **AMD/Intel capture on Win7:** GDI fallback deferred per branch decision.
2. **Toolchain / runtime:** CI builds `MSYS2 UCRT64` + Qt6-static + `cppwinrt` + C++23 + Boost 1.92. UCRT needs `KB2999226` on Win7 SP1 (now checked at install, see below); Qt6 requires 10 1809+ (tray must be disabled with `-DSUNSHINE_ENABLE_TRAY=OFF` on Win7); Python 3.14 / Node LTS / .NET 10 (WiX) build hosts are 10+. Win7 builds use `SUNSHINE_ENABLE_WGC=OFF`, `SUNSHINE_ENABLE_TRAY=OFF`, and the Linux cross path below.
3. **Drivers:** ViGEmBus 1.17+ and Virtual HID Driver (paid license) are Win10-era. On Win7, expect keyboard/mouse via `SendInput` and Xbox360/DS4 only if ViGEm 1.17 installs; otherwise gamepads unavailable.
4. **Audio:** `audio.cpp` only probes `Steam/drivers/Windows10/...inf`. A Win7 INF path + `DiInstallDriverW` fallback is needed.
5. **Encoder prebuilts:** `third-party/build-deps` FFmpeg tarballs (`mfplat`, `mfuuid`, oneVPL) are Win10-era. Verify they load on 7 or rebuild with Win7 SDK.
6. **Vista:** same as Win7 plus older D3D11 (11.0 via platform update), weaker WASAPI. Deferred until 7 + 8.x are green.

## Installers on legacy systems

- `sunshine-setup.ps1` runs a `Test-LegacyPrerequisites` step first (both MSI
  custom actions and NSIS `nsExec` go through it): aborts below Windows 7 and
  below PowerShell 5 (stock Win7 ships PS 2.0 → install WMF 5.1,
  `https://aka.ms/wmf5download`; the script itself uses `Write-Information`),
  warns when `ucrtbase.dll` is missing (Win7/8.x need KB2999226,
  `https://learn.microsoft.com/en-us/cpp/windows/universal-crt-deployment`),
  and notes the VC++ redist situation (MinGW `-static` builds only need the OS
  UCRT; MSVC builds need the matching VC redist).
- No .NET Framework check exists, intentionally: nothing in Sunshine's Windows
  runtime uses .NET (no managed code in `src/`, `tools/`, or the installer
  actions; WiX `dotnet` is build-host tooling only).
- WiX (`wix.template.in`) blocks below `VersionNT >= 601` with a plain-language
  message. NSIS has no custom `WinVer` include in the CPack template, so it
  relies on the PS1 gate above.
- Full offline bundling of the UCRT/VC redist installers (WiX Burn bundle) is
  still open; current behavior fails fast with download locations.

## Building for legacy Windows

Native (MSYS2, as upstream documents in `docs/building.md:166`):

```powershell
# 8.x (DDX, no WGC needed but harmless to keep):
cmake -B build -G Ninja -S . -DSUNSHINE_ENABLE_TRAY=ON

# 7 (no WGC, no tray until Qt story is solved):
cmake -B build -G Ninja -S . -DSUNSHINE_ENABLE_WGC=OFF -DSUNSHINE_ENABLE_TRAY=OFF
```

Cross from Linux (new on this branch):

Upstream states `docs/building.md:170` "Cross-compilation is not supported on
Windows. You must build on the target architecture." This branch adds a baseline
so the Windows target can be configured from a Linux host:

- `cmake/toolchain/windows-mingw64-x86_64.cmake`: `CMAKE_SYSTEM_NAME=Windows`,
  `x86_64-w64-mingw32-{gcc,g++,windres}`, target-only find roots,
  `STATIC_LIBRARY` try-compile (no target execution), mingw `pkg-config`
  wrapper when installed.
- `cmake/targets/common.cmake`: under `CMAKE_CROSSCOMPILING` the web-ui uses
  host-native `npm`/`node` directly (no `cmd /C call`, no `npm.cmd` lookup).
- `cmake/packaging/windows.cmake`: shader junction uses `cmake -E create_symlink`
  unless `CMAKE_HOST_WIN32` (where `mklink /J` is kept).
- `cmake/dependencies/windows.cmake`: when cross-compiling without a sysroot
  MinHook, it is fetched (`TsudaKageyu/minhook` v1.3.4) and built from source.
- Include case fixed repo-wide (`<Windows.h>` → `<windows.h>`) so
  case-sensitive Linux filesystems resolve the mingw headers.

### What to install (Arch Linux / CachyOS)

`tools/cross-mingw-setup-arch.sh` does all of this; the list below is what it
installs and why:

| Package (source) | Why |
|---|---|
| `base-devel cmake ninja mingw-w64-gcc nodejs npm python uv git curl pkgconf` (pacman) | Host tools + bare cross toolchain + web-ui + python tooling |
| `mingw-w64-pkg-config mingw-w64-zlib mingw-w64-openssl mingw-w64-boost mingw-w64-curl mingw-w64-opus mingw-w64-nlohmann-json` (AUR via `paru`) | OpenSSL/curl/opus/miniupnpc-deps, Boost 1.92, FFmpeg-independent headers |
| miniupnpc (built from `github.com/miniupnp/miniupnp` into `/usr/x86_64-w64-mingw32`) | No AUR cross package exists |
| Intel VPL dispatcher (built from `github.com/intel/libvpl` into the sysroot) | Supplies the `MFX*` symbols the `Windows-AMD64-ffmpeg.tar.gz` prebuilt expects at link; no AUR cross package exists |
| MinHook | Fetched at configure time by CMake (see above), no install needed |
| Qt / cppwinrt | Skipped: legacy cross uses `SUNSHINE_ENABLE_TRAY=OFF` (Qt6 needs Win10 1809+) and `SUNSHINE_ENABLE_WGC=OFF` (WinRT needs Win10+) |
| `node` LTS + `dotnet` 10 | Only for full native packaging; cross produces ZIP (`cpack -G ZIP`), NSIS/WiX stay Windows-host only |

```bash
# One-shot setup, then build:
./tools/cross-mingw-setup-arch.sh
cmake -B cmake-build-cross-mingw -S . \
  --toolchain cmake/toolchain/windows-mingw64-x86_64.cmake \
  -DSUNSHINE_ENABLE_WGC=OFF -DSUNSHINE_ENABLE_TRAY=OFF -DBUILD_TESTS=OFF -DBUILD_DOCS=OFF
cmake --build cmake-build-cross-mingw
```

Verified 2026-10-04: toolchain configures `GNU 16.2.0` cross compilers; the
FFmpeg prebuilt needs the sysroot VPL dispatcher (25 `MFX*` undefined symbols
without it). Debian/Ubuntu need `g++-mingw-w64-x86-64` plus equivalent
hand-built sysroot libraries; Fedora needs `mingw64-gcc-c++` plus the same.

Run `tools/dxgi` (now 7-safe) to verify enumeration before streaming.

## Roadmap

1. NvFBC Windows ToSys backend (done here): Win7/NVIDIA capture for software and NVENC encoding.
2. Installer offline bundling (WiX Burn) for UCRT/VC redist.
3. Win7 toolchain CI job reusing the cross path.
4. GDI fallback for AMD/Intel on 7 (deferred per branch decision).
5. Vista pass (only after 7 + 8.x are green).
