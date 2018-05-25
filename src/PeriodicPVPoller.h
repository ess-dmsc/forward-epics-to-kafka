#pragma once

namespace BrightnESS {
namespace ForwardEpicsToKafka {

using CallbackFunction = std::function<void()>;

class PeriodicPVPoller {
public:
  PeriodicPVPoller(std::chrono::milliseconds interval) : IntervalMS(interval){};

  void start() {
    Running = true;
    Thr = std::thread([=]() {
      while (Running) {
        std::this_thread::sleep_for(IntervalMS);
        {
          std::lock_guard<std::mutex> lock(CallbacksMutex);
          for (CallbackFunction Callback : Callbacks) {
            Callback();
          }
        }
      }
    });
    Thr.join();
  };

  void stop() { Running = false; };

  void addCallback(CallbackFunction callback) {
    std::lock_guard<std::mutex> lock(CallbacksMutex);
    Callbacks.push_back(callback);
  }

private:
  std::vector<CallbackFunction> Callbacks;
  bool Running = false;
  std::thread Thr;
  std::chrono::milliseconds IntervalMS;
  std::mutex CallbacksMutex;
};
}
}