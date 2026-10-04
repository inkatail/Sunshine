/**
 * @file src/platform/windows/os_version_check.h
 * @brief Pure Windows version predicates (no OS headers, testable on any host).
 *
 * @details Version mapping:
 * - 6.0 = Vista / Server 2008
 * - 6.1 = 7 / Server 2008 R2
 * - 6.2 = 8 / Server 2012
 * - 6.3 = 8.1 / Server 2012 R2
 * - 10.0+ = 10 / 11 and later
 */
#pragma once

namespace platf::win_legacy {
  /**
   * @brief Pure version predicate for Windows 8 / Server 2012 or newer (DDX floor).
   *
   * @param major Major version.
   * @param minor Minor version.
   * @return True when the version is at least 6.2.
   */
  constexpr inline bool is_win8_or_greater_version(unsigned long major, unsigned long minor) {
    return (major > 6) || (major == 6 && minor >= 2);
  }

  /**
   * @brief Pure version predicate for Windows 8.1 / Server 2012 R2 or newer.
   *
   * @param major Major version.
   * @param minor Minor version.
   * @return True when the version is at least 6.3.
   */
  constexpr inline bool is_win81_or_greater_version(unsigned long major, unsigned long minor) {
    return (major > 6) || (major == 6 && minor >= 3);
  }

  /**
   * @brief Pure version predicate for Windows 10 / 11 or newer (WGC floor).
   *
   * @param major Major version.
   * @param minor Minor version.
   * @return True when the version is at least 10.0.
   */
  constexpr inline bool is_win10_or_greater_version(unsigned long major, unsigned long /*minor*/) {
    return major >= 10;
  }

  /**
   * @brief Pure version predicate for exactly Windows 7 / Server 2008 R2.
   *
   * @param major Major version.
   * @param minor Minor version.
   * @return True when the version is exactly 6.1.
   */
  constexpr inline bool is_windows_7_version(unsigned long major, unsigned long minor) {
    return major == 6 && minor == 1;
  }
}  // namespace platf::win_legacy
