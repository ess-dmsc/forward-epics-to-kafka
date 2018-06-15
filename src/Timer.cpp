#include "Timer.h"

namespace Forwarder {

void Timer::start() {
  Running = true;
  TimerThread = std::thread(executionLoop, this);
  // Prevent the Timer being told to stop before it has got going
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
};

void Timer::triggerStop() { Running = false; };

void Timer::waitForStop() { TimerThread.join(); }

void Timer::addCallback(CallbackFunction Callback) {
  std::lock_guard<std::mutex> lock(CallbacksMutex);
  Callbacks.push_back(Callback);
}
}
