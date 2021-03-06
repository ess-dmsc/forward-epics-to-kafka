// SPDX-License-Identifier: BSD-2-Clause
//
// This code has been produced by the European Spallation Source
// and its partner institutes under the BSD 2 Clause License.
//
// See LICENSE.md at the top level for license information.
//
// Screaming Udder!                              https://esss.se

#include <RangeSet.h>
#include <gtest/gtest.h>

TEST(RangeSet, EmptySetToString) {
  RangeSet<int64_t> Ranges;
  EXPECT_EQ(Ranges.to_string(), "[]");
}

TEST(RangeSet, OneElementSetToString) {
  RangeSet<int64_t> Ranges;
  Ranges.set.insert({0, 1});
  EXPECT_EQ(Ranges.to_string(), "[[0, 1]]");
}

TEST(RangeSet, TwoElementSetToString) {
  RangeSet<int64_t> Ranges;
  Ranges.set.insert({0, 1});
  Ranges.set.insert({2, 3});
  EXPECT_EQ(Ranges.to_string(), "[[0, 1], [2, 3]]");
}
