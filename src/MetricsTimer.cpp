// SPDX-License-Identifier: BSD-2-Clause
//
// This code has been produced by the European Spallation Source
// and its partner institutes under the BSD 2 Clause License.
//
// See LICENSE.md at the top level for license information.
//
// Screaming Udder!                              https://esss.se

#include "MetricsTimer.h"

namespace Forwarder {

void MetricsTimer::collectMetrics() {
  if (Running) {
    {
      std::lock_guard<std::mutex> CallbackLock(CallbacksMutex);

    }
    AsioTimer.expires_at(AsioTimer.expires_at() + Period);
    AsioTimer.async_wait([this](std::error_code const & /*error*/) {
      // this->executeCallbacks();
    });
  }
}

void MetricsTimer::start() {
  Running = true;
  AsioTimer.async_wait(
      [this](std::error_code const & /*error*/) { ; });
  TimerThread = std::thread(&MetricsTimer::run, this);
}

void MetricsTimer::waitForStop() {
  Running = false;
  AsioTimer.cancel();
  TimerThread.join();
}

} // namespace Forwarder
