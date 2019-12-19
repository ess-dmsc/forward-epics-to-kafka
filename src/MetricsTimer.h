// SPDX-License-Identifier: BSD-2-Clause
//
// This code has been produced by the European Spallation Source
// and its partner institutes under the BSD 2 Clause License.
//
// See LICENSE.md at the top level for license information.
//
// Screaming Udder!                              https://esss.se

#pragma once
#include "logger.h"
#include <asio.hpp>
#include <atomic>
#include <chrono>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

namespace Forwarder {

using CallbackFunction = std::function<void()>;

/// Multiple callbacks can be registered with the Timer, which will repeatedly
/// execute them at a set interval
class MetricsTimer {
public:
  explicit MetricsTimer(std::chrono::milliseconds Interval)
      : IO(), Period(Interval), AsioTimer(IO, Period), Running(false) {}

  /// Perform status reports
  void collectMetrics();

  /// Starts the timer thread with a call to the callbacks
  void start();

  /// Blocks until the timer thread has stopped
  void waitForStop();

private:
  void run() { IO.run(); }
  asio::io_context IO;
  std::chrono::milliseconds Period;
  asio::steady_timer AsioTimer;
  std::atomic_bool Running;
  std::mutex CallbacksMutex;
  std::vector<CallbackFunction> Callbacks{};
  std::thread TimerThread;
};

} // namespace Forwarder
