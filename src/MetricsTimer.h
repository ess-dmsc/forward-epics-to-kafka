// SPDX-License-Identifier: BSD-2-Clause
//
// This code has been produced by the European Spallation Source
// and its partner institutes under the BSD 2 Clause License.
//
// See LICENSE.md at the top level for license information.
//
// Screaming Udder!                              https://esss.se

#pragma once
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

namespace Forwarder {

class Converter;

class MetricsTimer {
public:
  explicit MetricsTimer(std::chrono::milliseconds Interval,
                        MainOpt &ApplicationMainOptions,
                        std::shared_ptr<InstanceSet> &MainLoopKafkaInstanceSet)
      : IO(), Period(Interval), AsioTimer(IO, Period), Running(false),
        MainOptions(ApplicationMainOptions),
        KafkaInstanceSet(MainLoopKafkaInstanceSet) {
    this->start();
  }

  /// Blocks until the timer thread has stopped
  void waitForStop();

  std::unique_lock<std::mutex> get_lock_converters();

  void reportMetrics();

  ~MetricsTimer();

private:
  /// Starts the timer thread with a call to the callbacks
  void start();
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
  std::shared_ptr<InstanceSet> KafkaInstanceSet;
};

} // namespace Forwarder
