/**
 * @file src/platform/windows/os_version.h
 * @brief Windows version helpers for the windows-legacy branch.
 *
 * @details Uses RtlGetVersion from ntdll.dll instead of GetVersionEx or
 * VerifyVersionInfo so results do not depend on the application manifest.
 * RtlGetVersion has been available since Windows 2000 and reports the real
 * OS version even when the process has no supportedOS entries.
 *
 * Version mapping used here:
 * - 6.0 = Vista / Server 2008
 * - 6.1 = 7 / Server 2008 R2
 * - 6.2 = 8 / Server 2012
 * - 6.3 = 8.1 / Server 2012 R2
 * - 10.0+ = 10 / 11 and later
 *
 * Desktop Duplication (IDXGIOutput1::DuplicateOutput) requires 6.2+.
 * DuplicateOutput1 (IDXGIOutput5) requires 10.0+.
 * Windows.Graphics.Capture requires 10.0+ (1803 for monitor capture).
 */
#pragma once

// platform includes
#include <windows.h>

// local includes
#include "os_version_check.h"

namespace platf::win_legacy {
  /**
   * @brief Raw OS version triple reported by RtlGetVersion.
   */
  struct os_version_t {
    unsigned long major = 0;  ///< Major version.
    unsigned long minor = 0;  ///< Minor version.
    unsigned long build = 0;  ///< Build number.
  };

  /**
   * @brief Query the real OS version via RtlGetVersion.
   *
   * @return OS version triple. Returns zeros only if ntdll could not be queried.
   */
  inline os_version_t rtl_os_version() {
    // NOTE: Keep this struct local so we do not depend on winternl.h SDK versions.
    struct rtl_osversioninfoexw {
      ULONG dwOSVersionInfoSize;
      ULONG dwMajorVersion;
      ULONG dwMinorVersion;
      ULONG dwBuildNumber;
      ULONG dwPlatformId;
      WCHAR szCSDVersion[128];
      USHORT wServicePackMajor;
      USHORT wServicePackMinor;
      USHORT wSuiteMask;
      UCHAR wProductType;
      UCHAR wReserved;
    };

    using rtl_get_version_fn = LONG(WINAPI *)(rtl_osversioninfoexw *);
    static rtl_get_version_fn fn = nullptr;
    static bool resolved = false;
    if (!resolved) {
      resolved = true;
      if (auto ntdll = GetModuleHandleW(L"ntdll.dll")) {
        fn = reinterpret_cast<rtl_get_version_fn>(GetProcAddress(ntdll, "RtlGetVersion"));
      }
    }

    os_version_t version {};
    if (fn) {
      rtl_osversioninfoexw info {};
      info.dwOSVersionInfoSize = sizeof(info);
      if (fn(&info) == 0) {
        version.major = info.dwMajorVersion;
        version.minor = info.dwMinorVersion;
        version.build = info.dwBuildNumber;
      }
    }
    return version;
  }

  /**
   * @brief Cached OS version for the current process.
   *
   * @return OS version triple.
   */
  inline os_version_t os_version() {
    static const os_version_t cached = rtl_os_version();
    return cached;
  }

  /**
   * @brief Check for Windows 8 / Server 2012 or newer (Desktop Duplication floor).
   *
   * @return True when Desktop Duplication API may exist.
   */
  inline bool is_win8_or_greater() {
    const auto v = os_version();
    return is_win8_or_greater_version(v.major, v.minor);
  }

  /**
   * @brief Check for Windows 8.1 / Server 2012 R2 or newer.
   *
   * @return True when the OS is at least 6.3.
   */
  inline bool is_win81_or_greater() {
    const auto v = os_version();
    return is_win81_or_greater_version(v.major, v.minor);
  }

  /**
   * @brief Check for Windows 10 / 11 or newer (WGC floor).
   *
   * @return True when Windows.Graphics.Capture may exist.
   */
  inline bool is_win10_or_greater() {
    const auto v = os_version();
    return is_win10_or_greater_version(v.major, v.minor);
  }

  /**
   * @brief Check for exactly Windows 7 / Server 2008 R2.
   *
   * @return True when running on 6.1.
   */
  inline bool is_windows_7() {
    const auto v = os_version();
    return is_windows_7_version(v.major, v.minor);
  }
}  // namespace platf::win_legacy
