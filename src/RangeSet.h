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
    fmt::MemoryWriter mw;
    mw.write("[");
    int i1 = 0;
    for (auto &x : set) {
      if (i1 > 0) {
        mw.write(", ");
      }
      mw.write("[{}, {}]", x.first, x.second);
      ++i1;
      if (i1 > 100) {
        mw.write(" ...");
        break;
      }
    }
    mw.write("]\0");
    return std::string(mw.c_str());
  }

  std::set<std::pair<T, T>> set;
  std::mutex Mutex;
};
