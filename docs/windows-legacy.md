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
| Desktop Duplication (DDX) | Windows 8+ (`IDXGIOutput1::DuplicateOutput`) | **Enabled on 8.x/10+, disabled on 7/Vista.** `duplication_t::init()` and `test_dxgi_duplication()` fail fast pre-8 via `os_version.h` (`RtlGetVersion`). `platf::display()` skips DDX when `!is_win8_or_greater()`. |
| Windows.Graphics.Capture (WGC) | Windows 10+ (1803 for monitors) | **Enabled on 10+ only, optional at compile time.** `wgc_capture_t::init()` fails fast pre-10. `platf::display()` skips WGC pre-10. Build with `-DSUNSHINE_ENABLE_WGC=OFF` to exclude `display_wgc.cpp`, `winrt/windows.graphics.capture.h`, and `Windowsapp.lib` for Win7/8.x builds. |
| NvFBC for Windows | NVIDIA driver, Vista+ | **Not implemented yet.** The existing NvFBC code (`src/platform/linux/cuda.cpp`, `third-party/nvfbc/NvFBC.h`) is **Linux-only** (X11 + CUDA + Linux NvFBC API) and cannot be reused as-is on Windows. Windows NvFBC uses a different SDK (`NvFBCToDx9Vid` / DX11 paths via GRID SDK). This is the planned high-perf path for NVIDIA on Win7. |
| GDI / DWM fallback | Vista+ (`BitBlt`, `GetDC`, DWM) | **Not implemented yet.** Required for AMD/Intel on Win7 (no DDX, no WGC, no NvFBC). Planned next: `display_gdi_*` backends that capture via GDI to system memory and upload to D3D11. |

Net effect today:

- **Windows 7:** both DDX and WGC are disabled by design. The process starts (no hard Win10 imports) but `platf::display()` returns `nullptr` until NvFBC-Windows or GDI lands. This is intentional: previous code would install then fail at runtime with misleading `DuplicateOutput` errors.
- **Windows 8.0/8.1:** DDX works (`DuplicateOutput`, not `DuplicateOutput1`). WGC is skipped. Win10-only code paths (Output6 HDR, `win32u.dll` hook, `SetThreadDescription`, `SetProcessDpiAwarenessContext`, high-resolution timer flag, `JOB_LIST`) are guarded.
- **Windows 10/11:** unchanged behavior (DDX first, WGC fallback).

### Loader / API guards in this iteration

- `src/platform/windows/os_version.h` (new): `RtlGetVersion`-based helpers (`is_win8_or_greater`, `is_win10_or_greater`, `is_windows_7`). No manifest dependency.
- `display_base.cpp`: early-out in `duplication_t::init` + `test_dxgi_duplication`; `win32u.dll` hook only on 10+ with `MH_CreateHookApi` result checked; factory logs when backends are disabled.
- `display_wgc.cpp`: early-out pre-10 before any WinRT activation.
- `display.h` + CMake: `SUNSHINE_ENABLE_WGC` (default `ON`). When `OFF`, defines `SUNSHINE_NO_WGC`, excludes `display_wgc.cpp`, omits `Windowsapp.lib` (UWP umbrella, Win10+).
- `misc.cpp`: `SetThreadDescription` via `GetProcAddress` (no-op on 7/8.x); `PROC_THREAD_ATTRIBUTE_JOB_LIST` failure falls back to no job tracking instead of breaking launch; `CREATE_WAITABLE_TIMER_HIGH_RESOLUTION` has an SDK fallback define + runtime fallback.
- `tools/dxgi.cpp`: `WINVER/_WIN32_WINNT=0x0601`, DPI-awareness-context via `GetProcAddress` with `SetProcessDPIAware` fallback.
- `input.cpp`: `WINVER/_WIN32_WINNT=0x0601` (no Win10-only static imports in this TU).

`NTDDI_VERSION=NTDDI_WIN10` in `misc.cpp` is intentionally left as-is: it keeps
`IDXGIOutput5/6` declarations visible for 8.x/10+ builds. Downlevel execution is
handled by `QueryInterface` / `GetProcAddress` guards, not by lowering the SDK.

## Known remaining Win7 blockers (not fixed here)

These must be addressed before a Win7 binary is usable. Listed so the next
commits have a checklist:

1. **No capture backend on Win7 yet** (see table). Priority: GDI fallback, then NvFBC-Windows.
2. **Toolchain / runtime:** CI builds `MSYS2 UCRT64` + Qt6-static + `cppwinrt` + C++23 + Boost 1.92. UCRT needs `KB2999226` on Win7 SP1; Qt6 requires 10 1809+ (tray must be disabled with `-DSUNSHINE_ENABLE_TRAY=OFF` on Win7); Python 3.14 / Node LTS / .NET 10 (WiX) build hosts are 10+. Win7 builds need `mingw-w64-x86_64-msvcrt` or a pinned older UCRT, older Qt (5.x) or no tray, and `SUNSHINE_ENABLE_WGC=OFF`.
3. **Drivers:** ViGEmBus 1.17+ and Virtual HID Driver (paid license) are Win10-era. On Win7, expect keyboard/mouse via `SendInput` and Xbox360/DS4 only if ViGEm 1.17 installs; otherwise gamepads unavailable.
4. **Audio:** `audio.cpp` only probes `Steam/drivers/Windows10/...inf`. A Win7 INF path + `DiInstallDriverW` fallback is needed.
5. **Encoder prebuilts:** `third-party/build-deps` FFmpeg tarballs (`mfplat`, `mfuuid`, oneVPL) are Win10-era. Verify they load on 7 or rebuild with Win7 SDK.
6. **Installer:** WiX v4 + `sunshine-setup.ps1` have no OS gate or UCRT/`KB2999226` bootstrap. Add `VersionNT` / `RtlGetVersion` check + redist install before claiming Win7 support.
7. **Vista:** same as Win7 plus older D3D11 (11.0 via platform update), no `CreateWaitableTimerEx` flags, weaker WASAPI. Deferred until 7 + 8.x are green.

## Building for legacy Windows

```powershell
# 8.x (DDX, no WGC needed but harmless to keep):
cmake -B build -G Ninja -S . -DSUNSHINE_ENABLE_TRAY=ON

# 7 (no WGC, no tray until Qt story is solved):
cmake -B build -G Ninja -S . -DSUNSHINE_ENABLE_WGC=OFF -DSUNSHINE_ENABLE_TRAY=OFF
```

Run `tools/dxgi` (now 7-safe) to verify enumeration before streaming.

## Roadmap

1. Land this gating (done here) so 8.x works and 7 fails with a clear log.
2. Implement `display_gdi_ram_t` / `display_gdi_vram_t` (GDI `BitBlt` → D3D11 upload, cursor via `GetCursorInfo` + `DrawIconEx`).
3. Implement Windows NvFBC backend for NVIDIA (separate SDK headers, not `third-party/nvfbc/NvFBC.h`).
4. Win7 toolchain docs + CI job (`msvcrt`, `SUNSHINE_ENABLE_WGC=OFF`, `SUNSHINE_ENABLE_TRAY=OFF`).
5. Installer OS gate + UCRT bootstrap.
6. Vista pass (only after 7 + 8.x are green).
