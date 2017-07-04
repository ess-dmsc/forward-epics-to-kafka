#include "Ring.h"
#include "logger.h"
#include "epics-to-fb.h"
#include "ConversionWorker.h"

namespace BrightnESS {
namespace ForwardEpicsToKafka {

static uint32_t const cap_max = 1024 * 1024;

// Switchable for testing
#define LOCK() ulock lock(mx);

template <typename TP> Ring<TP>::Ring() {
  LOCK();
  resize_unsafe(16);
}

template <typename TP> Ring<TP>::Ring(uint32_t n) {
  LOCK();
  resize_unsafe(n);
}

template <typename TP> int Ring<TP>::resize(uint32_t n) {
  LOCK();
  return resize_unsafe(n);
}

template <typename TP> int Ring<TP>::resize_unsafe(uint32_t n) {
  n = (std::min)(n, cap_max);
  if (n > capacity_unsafe()) {
    // CLOG(7, 1, "resize {}", n);
    if (iW >= iR) {
      vec.resize(n + 1);
      iRmax = vec.size();
      return 0;
    } else {
      uint32_t n1 = vec.size();
      vec.resize(n + 1);
      for (uint32_t i1 = 0; i1 < iW; ++i1) {
        vec[n1] = std::move(vec[i1]);
        ++n1;
        if (n1 >= vec.size())
          n1 = 0;
      }
      iW = n1;
      return 0;
    }
  } else if (n < capacity_unsafe()) {
    // Not supported, nor wanted
    return 1;
  }
  // same size, nothing to be done.
  return 0;
}

template <typename TP> int Ring<TP>::push(TP &x) {
  LOCK();
  return push_unsafe(x);
}

template <typename TP> int Ring<TP>::push_unsafe(TP &x) {
  if (iW >= iR) {
    if (iW < vec.size() - 1 || iR > 0) {
      vec[iW] = std::move(x);
      inc_W();
      return 0;
    }
  } else {
    if (iW < iR - 1) {
      vec[iW] = std::move(x);
      inc_W();
      return 0;
    }
  }
  // full
  return 1;
}

template <typename TP> int Ring<TP>::push_enlarge(TP &x) {
  LOCK();
  return push_enlarge_unsafe(x);
}

template <typename TP> int Ring<TP>::push_enlarge_unsafe(TP &p) {
  auto x = push_unsafe(p);
  if (x == 0)
    return x;
  resize_unsafe(capacity_unsafe() * 4 / 3);
  return push_unsafe(p);
}

template <typename TP> std::pair<int, TP> Ring<TP>::pop() {
  LOCK();
  return pop_unsafe();
}

template <typename TP> std::pair<int, TP> Ring<TP>::pop_unsafe() {
  if (iR < iW) {
    auto &v = vec[iR];
    inc_R();
    return { 0, std::move(v) };
  } else if (iR > iW) {
    auto &v = vec[iR];
    inc_R();
    // currently, we never invalidate already written items
    return { 0, std::move(v) };
  }
  // empty
  return { 1, nullptr };
}

template <typename TP> void Ring<TP>::inc_W() {
  ++iW;
  if (iW >= vec.size()) {
    iRmax = iW;
    iW = 0;
  }
}

template <typename TP> void Ring<TP>::inc_R() {
  ++iR;
  if (iR >= iRmax) {
    iR = 0;
  }
}

template <typename TP> uint32_t Ring<TP>::size() {
  LOCK();
  return size_unsafe();
}

template <typename TP> uint32_t Ring<TP>::size_unsafe() {
  if (iW >= iR) {
    return iW - iR;
  } else {
    return vec.size() - iR + iW;
  }
}

template <typename TP> uint32_t Ring<TP>::capacity() {
  LOCK();
  return capacity_unsafe();
}

template <typename TP> uint32_t Ring<TP>::capacity_unsafe() {
  auto n = vec.size();
  if (n == 0)
    return 0;
  return n - 1;
}

template <typename TP>
int32_t Ring<TP>::fill_from(Ring &r, uint32_t const max) {
  LOCK();
  ulock lock2(r.mx);
  uint32_t n1 = 0;
  uint32_t n2 = r.size_unsafe();
  uint32_t n3 = capacity_unsafe() - size_unsafe();
  while (n1 < max && n1 < n3 && n1 < n2) {
    auto e = r.pop_unsafe();
    if (e.first != 0) {
      LOG(7, "empty? should not happen");
      break;
    }
    auto x = push_unsafe(e.second);
    if (x != 0) {
      LOG(7, "full? should not happen");
      break;
    }
    n1 += 1;
  }
  return n1;
}

template <typename TP> std::vector<char> Ring<TP>::to_vec() {
  LOCK();
  return to_vec_unsafe();
}

template <typename TP> std::vector<char> Ring<TP>::to_vec_unsafe() {
  std::vector<char> ret;
  uint32_t i1 = 0;
  static char const *col1[] = { "",              "\x1b[1;31m",
                                "\x1b[1;32m",    "\x1b[1;35m",
                                "\x1b[40;97m",   "\x1b[40;1;31m",
                                "\x1b[40;1;32m", "\x1b[40;1;35m", };
  static char const *col2[] = { "", "\x1b[0;49m", };
  for (auto &e : vec) {
    int n1 = 0;
    int n2 = 1;
    if (i1 == iRmax - 1) {
      n1 = 4;
      if (i1 == iW && i1 == iR) {
        n1 = 7;
      } else if (i1 == iR) {
        n1 = 6;
      } else if (i1 == iW) {
        n1 = 5;
      }
      n2 = 1;
    } else {
      if (i1 == iW && i1 == iR) {
        n1 = 3;
        n2 = 1;
      } else if (i1 == iR) {
        n1 = 2;
        n2 = 1;
      } else if (i1 == iW) {
        n1 = 1;
        n2 = 1;
      }
    }
    uint16_t p1 = 0;
    if (e) {
      if (formatter)
        p1 = formatter(e);
      else
        p1 = 0xffff;
    }
    auto s1 = fmt::format("{}{:04x}{} ", col1[n1], p1, col2[n2]);
    std::copy(s1.data(), s1.data() + s1.size(), std::back_inserter(ret));
    ++i1;
    if (i1 % 16 == 0)
      ret.push_back('\n');
  }
  ret.push_back(0);
  return ret;
}

template <typename TP> typename Ring<TP>::ulock Ring<TP>::lock() {
  return ulock(mx);
}

template class Ring<std::unique_ptr<FlatBufs::EpicsPVUpdate> >;
template class Ring<std::unique_ptr<ConversionWorkPacket> >;
}
}
