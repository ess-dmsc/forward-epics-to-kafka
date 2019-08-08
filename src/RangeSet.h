// SPDX-License-Identifier: BSD-2-Clause
//
// This code has been produced by the European Spallation Source
// and its partner institutes under the BSD 2 Clause License.
//
// See LICENSE.md at the top level for license information.
//
// Screaming Udder!                              https://esss.se

#pragma once

#include <algorithm>
#include <fmt/format.h>
#include <memory>
#include <mutex>
#include <set>

/// A set of continuous inclusive ranges.
template <typename T> struct RangeSet {
  size_t size() {
    std::lock_guard<std::mutex> lock(Mutex);
    return set.size();
  }

  std::string to_string() {
    std::lock_guard<std::mutex> lock(Mutex);
    fmt::memory_buffer mw;
    fmt::format_to(mw, "[");
    int i1 = 0;
    for (auto &x : set) {
      if (i1 > 0) {
        fmt::format_to(mw, ", ");
      }
      fmt::format_to(mw, "[{}, {}]", x.first, x.second);
      ++i1;
      if (i1 > 100) {
        fmt::format_to(mw, " ...");
        break;
      }
    }
    fmt::format_to(mw, "]");
    return std::string(mw.data(), mw.size());
  }

  std::set<std::pair<T, T>> set;
  std::mutex Mutex;
};
