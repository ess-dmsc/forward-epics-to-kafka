#include "ConversionWorker.h"
#include "Forwarder.h"
#include "logger.h"
#include <chrono>
#include <thread>

namespace Forwarder {

ConversionWorkPacket::~ConversionWorkPacket() {
  if (stream) {
    cp->transit--;
  }
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
    auto qs = queue.size_approx();
    if (qs == 0) {
      auto qf = queue.MAX_SUBQUEUE_SIZE - qs;
      scheduler->fill(queue, qf, id);
    }
    while (true) {
      std::unique_ptr<ConversionWorkPacket> p;
      bool found = queue.try_dequeue(p);
      if (!found)
        break;
      auto cwp = std::move(p);
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

std::atomic<uint32_t> ConversionWorker::s_id{0};

ConversionScheduler::ConversionScheduler(Forwarder *main) : main(main) {}

int ConversionScheduler::fill(
    moodycamel::ConcurrentQueue<std::unique_ptr<ConversionWorkPacket>> &queue,
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
    auto track_seq_data = [&](uint64_t seq_data) {};
    auto n1 = main->streams[sid]->fill_conversion_work(queue, nfm - nfc,
                                                       track_seq_data);
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
