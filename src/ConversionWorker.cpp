#include "ConversionWorker.h"
#include "Forwarder.h"
#include "logger.h"
#include <chrono>
#include <thread>

namespace Forwarder {

ConversionWorkPacket::~ConversionWorkPacket() {
  if (stream) {
    Path->transit--;
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
  auto MinTimeDifference = MS(100);
  auto TimeBegin = CLK::now();
  while (do_run) {
    auto qs = queue.size_approx();
    if (qs == 0) {
      auto qf = queue.MAX_SUBQUEUE_SIZE - qs;
      scheduler->fill(queue, qf, id);
    }
    while (true) {
      std::unique_ptr<ConversionWorkPacket> Packet;
      if (!queue.try_dequeue(Packet)) {
        break;
      }
      Packet->Path->emit(std::move(Packet->Update));
    }
    auto TimeEnd = CLK::now();
    auto TimeDifference = std::chrono::duration_cast<MS>(TimeEnd - TimeBegin);
    if (TimeDifference < MinTimeDifference) {
      std::this_thread::sleep_for(MinTimeDifference - TimeDifference);
    }
    TimeBegin = TimeEnd;
  }
  return 0;
}

std::atomic<uint32_t> ConversionWorker::s_id{0};

ConversionScheduler::ConversionScheduler(Forwarder *main) : main(main) {}

int ConversionScheduler::fill(
    moodycamel::ConcurrentQueue<std::unique_ptr<ConversionWorkPacket>> &queue,
    uint32_t const nfm, uint32_t wid) {
  std::lock_guard<std::mutex> lock(mx);
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
    auto n1 = main->streams[sid]->fillConversionQueue(queue, nfm - nfc);
    if (n1 > 0) {
      Logger->debug("Give worker {:2}  items: {:3}  stream: {:3}", wid, n1,
                    sid);
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
  Logger->info("~ConversionScheduler");
}
} // namespace Forwarder
