#pragma once

#include <algorithm>
#include <fmt/format.h>
#include <memory>
#include <mutex>
#include <set>

/// Represents an inclusive range.
template <typename T> class Range {
public:
  Range(T a, T b) : a(a), b(b) {}
  T a;
  T b;
};

template <typename T>
constexpr bool operator<(Range<T> const &a, Range<T> const &b) {
  return (a.a < b.a || (a.a == b.a && a.b < b.b));
}

/// Test if the given ranges form together a gapless range.
template <typename T> bool is_gapless(Range<T> const &a, Range<T> const &b) {
  if (!(a < b)) {
    throw std::runtime_error("expect a < b");
  }
  if (a.b + 1 >= b.a) {
    return true;
  }
  return false;
}

/// Merge the given ranges into a new range.
template <typename T> Range<T> merge(Range<T> const &a, Range<T> const &b) {
  if (!(a < b)) {
    throw std::runtime_error("expect a < b");
  }
  return Range<T>(a.a, std::max(a.b, b.b));
}

/// A set of continuous inclusive ranges.
template <typename T> class RangeSet {
public:
  void insert(T k) {
    std::unique_lock<std::mutex> lock(mx);
    set.emplace(k, k);
    while (true) {
      auto a1 = std::adjacent_find(set.begin(), set.end(), is_gapless<T>);
      if (a1 == set.end()) {
        break;
      } else {
        auto a2 = a1;
        ++a2;
        auto a3 = merge(*a1, *a2);
        set.erase(a1);
        set.erase(a2);
        set.insert(a3);
      }
    }
  }

  size_t size() {
    std::unique_lock<std::mutex> lock(mx);
    return set.size();
  }

  std::string to_string() {
    std::unique_lock<std::mutex> lock(mx);
    fmt::MemoryWriter mw;
    mw.write("[");
    int i1 = 0;
    for (auto &x : set) {
      if (i1 > 0) {
        mw.write(", ");
      }
      mw.write("[{}, {}]", x.a, x.b);
      ++i1;
      if (i1 > 100) {
        mw.write(" ...");
        break;
      }
    }
    mw.write("]\0");
    return std::string(mw.c_str());
  }

  std::set<Range<T>> set;
  std::mutex mx;
};
