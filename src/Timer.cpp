#include "Timer.h"

namespace Forwarder {

void Timer::start() {
  Running = true;
  TimerThread = std::thread(executionLoop, this);
};

void Timer::stop() {
  Running = false;
  TimerThread.join();
};

void Timer::addCallback(CallbackFunction Callback) {
  std::lock_guard<std::mutex> lock(CallbacksMutex);
  Callbacks.push_back(Callback);
}
}
