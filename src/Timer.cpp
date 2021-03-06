// SPDX-License-Identifier: BSD-2-Clause
//
// This code has been produced by the European Spallation Source
// and its partner institutes under the BSD 2 Clause License.
//
// See LICENSE.md at the top level for license information.
//
// Screaming Udder!                              https://esss.se

#include "Timer.h"

namespace Forwarder {

void Timer::executeCallbacks() {
  if (Running) {
    {
      std::lock_guard<std::mutex> CallbackLock(CallbacksMutex);
      for (CallbackFunction const &Callback : Callbacks) {
        Callback();
      }
    }
    AsioTimer.expires_at(AsioTimer.expires_at() + Period);
    AsioTimer.async_wait([this](std::error_code const & /*error*/) {
      this->executeCallbacks();
    });
  }
}

void Timer::start() {
  Running = true;
  AsioTimer.async_wait(
      [this](std::error_code const & /*error*/) { this->executeCallbacks(); });
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
