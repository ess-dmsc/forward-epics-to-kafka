#pragma once

namespace BrightnESS {
namespace ForwardEpicsToKafka {

class PeriodicPVPoller {
  PeriodicPVPoller(std::chrono::milliseconds interval)
      : interval_ms(interval){};

  void start() {
    running = true;
    thr = std::thread([=]() {
      while (running) {
        std::this_thread::sleep_for(interval_ms);
        // do/call something here
      }
    });
    thr.join();
  };

  void stop() { running = false; };

private:
  bool running = false;
  std::thread thr;
  std::chrono::milliseconds interval_ms;
};
}
}