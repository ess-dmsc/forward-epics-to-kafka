#include "Timer.h"

namespace Forwarder {

void Timer::executeCallbacks() {
  if (Running) {
    {
      std::lock_guard<std::mutex> CallbackLock(CallbacksMutex);
      for (const CallbackFunction &Callback : Callbacks) {
        Callback();
      }
    }
    AsioTimer.expires_at(AsioTimer.expires_at() + Period);
    AsioTimer.async_wait([this](const std::error_code & /*error*/) {
      this->executeCallbacks();
    });
  }
}

void Timer::start() {
  Running = true;
  AsioTimer.async_wait(
      [this](const std::error_code & /*error*/) { this->executeCallbacks(); });
  TimerThread = std::thread(&Timer::run, this);
}

void Timer::waitForStop() {
  Running = false;
  AsioTimer.cancel();
  TimerThread.join();
}

void Timer::addCallback(CallbackFunction const &Callback) {
  std::lock_guard<std::mutex> lock(CallbacksMutex);
  Callbacks.push_back(Callback);
}

} // namespace Forwarder
