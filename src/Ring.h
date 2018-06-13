#pragma once
#include "logger.h"
#include <array>
#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace Forwarder {

class Stream;

template <typename TP> class Ring {
public:
  using mutex = std::mutex;
  using ulock = std::unique_lock<mutex>;
  Ring();
  explicit Ring(uint32_t n);
  int resize(uint32_t);
  int resize_unsafe(uint32_t);
  int push(TP &x);
  int push_unsafe(TP &x);
  int push_enlarge(TP &x);
  int push_enlarge_unsafe(TP &p);
  std::pair<int, TP> pop();
  std::pair<int, TP> pop_unsafe();
  uint32_t size();
  uint32_t size_unsafe();
  uint32_t capacity();
  uint32_t capacity_unsafe();
  int32_t fill_from(Ring &r, uint32_t max);
  std::vector<char> to_vec();
  std::vector<char> to_vec_unsafe();
  void inc_W();
  void inc_R();
  uint16_t (*formatter)(TP &) = nullptr;
  ulock lock();

private:
  mutex mx;
  uint32_t iW = 0;
  uint32_t iR = 0;
  uint32_t iRmax = 0;
  std::vector<TP> vec;
  friend class Stream;
};
}
