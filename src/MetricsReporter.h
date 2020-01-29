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

class MetricsReporter {
public:
  explicit MetricsReporter(
      std::chrono::milliseconds Interval, MainOpt &ApplicationMainOptions,
      std::shared_ptr<InstanceSet> &MainLoopKafkaInstanceSet)
      : IO(), Period(Interval), AsioTimer(IO, Period),
        MainOptions(ApplicationMainOptions),
        KafkaInstanceSet(MainLoopKafkaInstanceSet) {
    Logger->trace("Starting the MetricsTimer");
    AsioTimer.async_wait([this](std::error_code const &Error) {
      if (Error != asio::error::operation_aborted) {
        this->reportMetrics();
      }
    });
    MetricsThread = std::thread(&MetricsReporter::run, this);
  }

  std::unique_lock<std::mutex> get_lock_converters();

  void reportMetrics();

  ~MetricsReporter();

private:
  void run() { IO.run(); }
  asio::io_context IO;
  std::chrono::milliseconds Period;
  asio::steady_timer AsioTimer;
  std::thread MetricsThread;
  MainOpt &MainOptions;
  SharedLogger Logger = getLogger();
  std::map<std::string, std::weak_ptr<Converter>> converters;
  std::mutex converters_mutex;
  std::shared_ptr<InstanceSet> KafkaInstanceSet;
};

} // namespace Forwarder
