/**
 * @file tests/unit/test_os_version.cpp
 * @brief Test Windows legacy version predicates (host-independent).
 */
// test includes
#include "../tests_common.h"

// local includes
#include "src/platform/windows/os_version_check.h"

using namespace platf::win_legacy;

TEST(OsVersionCheckTest, VistaHasNeitherDdxNorWgc) {
  EXPECT_FALSE(is_win8_or_greater_version(6, 0));
  EXPECT_FALSE(is_win81_or_greater_version(6, 0));
  EXPECT_FALSE(is_win10_or_greater_version(6, 0));
  EXPECT_FALSE(is_windows_7_version(6, 0));
}

TEST(OsVersionCheckTest, Windows7HasNeitherDdxNorWgc) {
  EXPECT_FALSE(is_win8_or_greater_version(6, 1));
  EXPECT_FALSE(is_win81_or_greater_version(6, 1));
  EXPECT_FALSE(is_win10_or_greater_version(6, 1));
  EXPECT_TRUE(is_windows_7_version(6, 1));
}

TEST(OsVersionCheckTest, Windows8HasDdxButNotWgc) {
  EXPECT_TRUE(is_win8_or_greater_version(6, 2));
  EXPECT_FALSE(is_win81_or_greater_version(6, 2));
  EXPECT_FALSE(is_win10_or_greater_version(6, 2));
  EXPECT_FALSE(is_windows_7_version(6, 2));
}

TEST(OsVersionCheckTest, Windows81HasDdxButNotWgc) {
  EXPECT_TRUE(is_win8_or_greater_version(6, 3));
  EXPECT_TRUE(is_win81_or_greater_version(6, 3));
  EXPECT_FALSE(is_win10_or_greater_version(6, 3));
  EXPECT_FALSE(is_windows_7_version(6, 3));
}

TEST(OsVersionCheckTest, Windows10HasBothDdxAndWgc) {
  EXPECT_TRUE(is_win8_or_greater_version(10, 0));
  EXPECT_TRUE(is_win81_or_greater_version(10, 0));
  EXPECT_TRUE(is_win10_or_greater_version(10, 0));
  EXPECT_FALSE(is_windows_7_version(10, 0));
}

TEST(OsVersionCheckTest, Windows11HasBothDdxAndWgc) {
  EXPECT_TRUE(is_win8_or_greater_version(10, 0));
  EXPECT_TRUE(is_win10_or_greater_version(10, 0));
}
