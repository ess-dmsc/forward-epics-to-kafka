#pragma once
#include "EpicsPVUpdate.h"
#include "RangeSet.h"
#include "Stream.h"
#include <atomic>
#include <concurrentqueue/concurrentqueue.h>
#include <mutex>
#include <thread>

namespace Forwarder {

class Forwarder;
class ConversionScheduler;
class ConversionPath;
class Stream;

struct ConversionWorkPacket {
  ~ConversionWorkPacket();
  std::shared_ptr<FlatBufs::EpicsPVUpdate> up;
  ConversionPath *cp = nullptr;
  Stream *stream = nullptr;
};

class ConversionWorker {
public:
  ConversionWorker(ConversionScheduler *scheduler, uint32_t queue_size)
      : queue(queue_size), id(s_id++), scheduler(scheduler) {}
  int start();
  int stop();
  int run();

private:
  moodycamel::ConcurrentQueue<std::unique_ptr<ConversionWorkPacket>> queue;
  std::atomic<uint32_t> do_run{0};
  static std::atomic<uint32_t> s_id;
  uint32_t id;
  std::thread thr;
  ConversionScheduler *scheduler = nullptr;
};

class ConversionScheduler {
public:
  ConversionScheduler(Forwarder *main);
  ~ConversionScheduler();
  int fill(
      moodycamel::ConcurrentQueue<std::unique_ptr<ConversionWorkPacket>> &queue,
      uint32_t nfm, uint32_t wid);

private:
  Forwarder *main = nullptr;
  size_t sid = 0;
  std::mutex mx;
  RangeSet<uint64_t> seq_data_enqueued;
};
}
