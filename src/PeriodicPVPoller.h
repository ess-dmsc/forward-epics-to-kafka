#pragma once
#include <atomic>

namespace BrightnESS {
namespace ForwardEpicsToKafka {

using CallbackFunction = std::function<void()>;

///\class PeriodicPVPoller
///\brief Timer for the periodic updates.
/// Calls the callback from polling epics.
class PeriodicPVPoller {
public:
  explicit PeriodicPVPoller(std::chrono::milliseconds Interval)
      : Running(false), IntervalMS(Interval){};

  static void pollingLoop(PeriodicPVPoller *ThisPoller) {
    while (ThisPoller->Running) {
      std::this_thread::sleep_for(ThisPoller->IntervalMS);
      {
        std::lock_guard<std::mutex> lock(ThisPoller->CallbacksMutex);
        for (CallbackFunction Callback : ThisPoller->Callbacks) {
          Callback();
        }
      }
    }
  }

  ///\fn start()
  ///\brief starts the timer thread with a call to the callbacks
  void start() {
    Running = true;
    Thr = std::thread(pollingLoop, this);
    Thr.detach();
  };

  ///\fn stop
  ///\brief stops the timer thread.
  void stop() {
    Running = false;
    Thr.join();
  };

  ///\fn addCallback
  ///\brief adds a callback to the vector of callbacks for the timer loop to
  ///call
  ///\param Callback the Callback function to add to the Callbacks vector
  void addCallback(CallbackFunction Callback) {
    std::lock_guard<std::mutex> lock(CallbacksMutex);
    Callbacks.push_back(Callback);
  }

private:
  std::vector<CallbackFunction> Callbacks{};
  std::atomic_bool Running;
  std::thread Thr;
  std::chrono::milliseconds IntervalMS;
  std::mutex CallbacksMutex;
};
}
}