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
  MetricsReporter(std::chrono::milliseconds Interval,
                  MainOpt &ApplicationMainOptions,
                  std::shared_ptr<InstanceSet> &MainLoopKafkaInstanceSet);

  std::unique_lock<std::mutex> get_lock_converters();

  void reportMetrics();
  ~MetricsReporter();

  // Used for testing, so that we can poll to see when report iterations have
  // happened instead of relying on them happening within a short time frame
  std::atomic_uint64_t ReportIterations{0};

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
