// SPDX-License-Identifier: BSD-2-Clause
//
// This code has been produced by the European Spallation Source
// and its partner institutes under the BSD 2 Clause License.
//
// See LICENSE.md at the top level for license information.
//
// Screaming Udder!                              https://esss.se

#pragma once
#include "Converter.h"
#include "Kafka.h"
#include "MainOpt.h"
#include "logger.h"
#include <asio.hpp>
#include <atomic>
#include <chrono>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

class Converter;

namespace Forwarder {

using CallbackFunction = std::function<void()>;

/// Multiple callbacks can be registered with the Timer, which will repeatedly
/// execute them at a set interval
class MetricsTimer {
public:
  explicit MetricsTimer(std::chrono::milliseconds Interval,
                        MainOpt &ApplicationMainOptions,
                        std::atomic<std::chrono::milliseconds>
                            &MainLoopIterationExecutionDuration,
                        std::shared_ptr<InstanceSet> &MainLoopKafkaInstanceSet)
      : IO(), Period(Interval), AsioTimer(IO, Period), Running(false),
        MainOptions(ApplicationMainOptions),
        IterationExecutionDuration(MainLoopIterationExecutionDuration),
        KafkaInstanceSet(MainLoopKafkaInstanceSet) {}

  /// Starts the timer thread with a call to the callbacks
  void start();

  /// Blocks until the timer thread has stopped
  void waitForStop();

  std::unique_lock<std::mutex> get_lock_converters();

  void reportMetrics();

private:
  void run() { IO.run(); }
  asio::io_context IO;
  std::chrono::milliseconds Period;
  asio::steady_timer AsioTimer;
  std::atomic_bool Running;
  std::thread TimerThread;
  MainOpt &MainOptions;
  SharedLogger Logger = getLogger();
  std::map<std::string, std::weak_ptr<Converter>> converters;
  std::mutex converters_mutex;
  std::atomic<std::chrono::milliseconds> &IterationExecutionDuration;
  std::shared_ptr<InstanceSet> KafkaInstanceSet;
};

} // namespace Forwarder
