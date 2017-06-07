#include "ConversionWorker.h"
#include "logger.h"
#include <thread>
#include <chrono>
#include "Main.h"

namespace BrightnESS {
namespace ForwardEpicsToKafka {

ConversionWorkPacket::~ConversionWorkPacket() {
  if (stream) {
    cp->transit--;
  }
}

static uint16_t _fmt(std::unique_ptr<ConversionWorkPacket> &x) {
  if (!x->up)
    return 0xfff0;
  return (uint16_t)(((uint64_t)x->up.get()) >> 0);
}

ConversionWorker::ConversionWorker(ConversionScheduler *scheduler,
                                   uint32_t queue_size)
    : queue(queue_size), id(s_id++), scheduler(scheduler) {
  queue.formatter = _fmt;
}

int ConversionWorker::start() {
  do_run = 1;
  thr = std::thread([this] { run(); });
  return 0;
}

int ConversionWorker::stop() {
  do_run = 0;
  if (thr.joinable())
    thr.join();
  return 0;
}

int ConversionWorker::run() {
  using CLK = std::chrono::steady_clock;
  using MS = std::chrono::milliseconds;
  auto Dt = MS(100);
  auto t1 = CLK::now();
  while (do_run) {
    // CLOG(7, 4, "ConversionWorker  {}  RUNLOOP", id);
    auto qs = queue.size();
    if (qs == 0) {
      auto qf = queue.capacity() - qs;
      scheduler->fill(queue, qf, id);
      // auto s1 = queue.to_vec();
      // LOG(7, "got {} new packets:\n{}", n1, s1.data());
    }
    while (true) {
      auto p = queue.pop();
      if (p.first != 0)
        break;
      auto &cwp = p.second;
      cwp->cp->emit(std::move(cwp->up));
    }
    auto t2 = CLK::now();
    auto dt = std::chrono::duration_cast<MS>(t2 - t1);
    if (dt < Dt) {
      std::this_thread::sleep_for(Dt - dt);
    }
    t1 = t2;
  }
  return 0;
}

std::atomic<uint32_t> ConversionWorker::s_id{ 0 };

ConversionScheduler::ConversionScheduler(Main *main) : main(main) {}

int
ConversionScheduler::fill(Ring<std::unique_ptr<ConversionWorkPacket> > &queue,
                          uint32_t const nfm, uint32_t wid) {
  std::unique_lock<std::mutex> lock(mx);
  if (main->streams.size() == 0) {
    return 0;
  }
  auto lock_streams = main->get_lock_streams();
  uint32_t nfc = 0;
  if (sid >= main->streams.size()) {
    sid = 0;
  }
  auto sid0 = sid;
  while (nfc < nfm) {
    auto track_seq_data = [&](uint64_t seq_data) {
      if (false) {
        seq_data_enqueued.insert(seq_data);
      }
    };
    auto n1 = main->streams[sid]
                  ->fill_conversion_work(queue, nfm - nfc, track_seq_data);
    if (n1 > 0) {
      CLOG(7, 3, "Give worker {:2}  items: {:3}  stream: {:3}", wid, n1, sid);
    }
    nfc += n1;
    sid += 1;
    if (sid >= main->streams.size()) {
      sid = 0;
    }
    if (sid == sid0)
      break;
  }
  return nfc;
}

ConversionScheduler::~ConversionScheduler() {
  LOG(6, "~ConversionScheduler  seq_data_enqueued {}",
      seq_data_enqueued.to_string());
}
}
}
