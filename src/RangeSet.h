#pragma once
#include <algorithm>
#include <fmt/format.h>
#include <memory>
#include <mutex>
#include <rapidjson/document.h>
#include <set>

// Represent inclusive range
template <typename T> class Range {
public:
  Range(T a, T b) : a(a), b(b) {}
  T a;
  T b;
  bool check_consistent() {
    if (a > b) {
      throw std::runtime_error("not consistent");
    }
  }
  std::string to_s() const { return fmt::format("<Range {:3} {:3}>", a, b); }
};

template <typename T>
constexpr bool operator<(Range<T> const &a, Range<T> const &b) {
  return (a.a < b.a || (a.a == b.a && a.b < b.b));
}

template <typename T> bool is_gapless(Range<T> const &a, Range<T> const &b) {
  if (!(a < b)) {
    throw std::runtime_error("expect a < b");
  }
  if (a.b + 1 >= b.a) {
    return true;
  }
  return false;
}

template <typename T> Range<T> merge(Range<T> const &a, Range<T> const &b) {
  if (!(a < b)) {
    throw std::runtime_error("expect a < b");
  }
  return Range<T>(a.a, std::max(a.b, b.b));
}

template <typename T> inline void minmax(T *mm, T const &x) {
  T &min = mm[0];
  T &max = mm[1];
  if (min == -1 || x < min) {
    min = x;
  }
  if (max == -1 || x > max) {
    max = x;
  }
}

template <typename T> class RangeSet {
public:
  void insert(T k) {
    std::unique_lock<std::mutex> lock(mx);
    // DWLOG(3, "Before message insert");
    // for (auto & x : set) DWLOG(3, "{}", x.to_s());
    set.emplace(k, k);
    while (true) {
      auto a1 = std::adjacent_find(set.begin(), set.end(), is_gapless<T>);
      if (a1 == set.end()) {
        break;
      } else {
        auto a2 = a1;
        ++a2;
        // DWLOG(3, "have adjacent: {} and {}", a1->to_s(), a2->to_s());
        auto a3 = merge(*a1, *a2);
        set.erase(a1);
        set.erase(a2);
        set.insert(a3);
      }
    }
    // DWLOG(3, "After message insert");
    // for (auto & x : set) DWLOG(3, "{}", x.to_s());
  }

  rapidjson::Value to_json_value(rapidjson::Document &doc) {
    std::unique_lock<std::mutex> lock(mx);
    auto &a = doc.GetAllocator();
    rapidjson::Value v;
    v.SetArray();
    int i1 = 0;
    for (auto &rr : set) {
      rapidjson::Value w;
      w.SetArray();
      w.PushBack(rapidjson::Value(rr.a), a);
      w.PushBack(rapidjson::Value(rr.b), a);
      v.PushBack(w, a);
      i1 += 1;
      if (i1 > 128) {
        break;
      }
    }
    return v;
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
