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
#include "MainOpt.h"
#include "Kafka.h"
#include "Forwarder.h"
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
  explicit MetricsTimer(std::chrono::milliseconds Interval, MainOpt &main_opt)
      : IO(), Period(Interval), AsioTimer(IO, Period), Running(false), main_opt(main_opt) {}

  /// Perform status reports
  void collectMetrics();

  /// Starts the timer thread with a call to the callbacks
  void start();

  /// Blocks until the timer thread has stopped
  void waitForStop();

  static std::unique_lock<std::mutex> get_lock_converters();

    static void report_stats(int dt);

private:
  void run() { IO.run(); }
  asio::io_context IO;
  std::chrono::milliseconds Period;
  asio::steady_timer AsioTimer;
  std::atomic_bool Running;
  std::mutex CallbacksMutex;
  std::thread TimerThread;
  static std::unique_ptr<InstanceSet> KafkaInstanceSet;
  static MainOpt &main_opt;
  static SharedLogger Logger = getLogger();
static std::map<std::string, std::weak_ptr<Converter>> converters;
  static std::mutex converters_mutex;

};

} // namespace Forwarder
