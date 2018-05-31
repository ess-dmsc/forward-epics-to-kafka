#pragma once
#include "RangeSet.h"
#include "Ring.h"
#include "Stream.h"
#include "epics-to-fb.h"
#include <atomic>
#include <mutex>
#include <thread>

namespace Forwarder {

class Forwarder;
class ConversionScheduler;
class ConversionPath;
class Stream;

struct ConversionWorkPacket {
  ~ConversionWorkPacket();
  std::unique_ptr<FlatBufs::EpicsPVUpdate> up;
  ConversionPath *cp = nullptr;
  Stream *stream = nullptr;
};

class ConversionWorker {
public:
  ConversionWorker(ConversionScheduler *scheduler, uint32_t queue_size);
  int start();
  int stop();
  int run();

private:
  Ring<std::unique_ptr<ConversionWorkPacket>> queue;
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
  int fill(Ring<std::unique_ptr<ConversionWorkPacket>> &queue, uint32_t nfm,
           uint32_t wid);

private:
  Forwarder *main = nullptr;
  size_t sid = 0;
  std::mutex mx;
  RangeSet<uint64_t> seq_data_enqueued;
};
}
