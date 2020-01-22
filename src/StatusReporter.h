// SPDX-License-Identifier: BSD-2-Clause
//
// This code has been produced by the European Spallation Source
// and its partner institutes under the BSD 2 Clause License.
//
// See LICENSE.md at the top level for license information.
//
// Screaming Udder!                              https://esss.se

#pragma once

#include "MainOpt.h"
#include "Streams.h"
#include "logger.h"
#include <asio.hpp>

namespace Forwarder {

class StatusReporter {
public:
  explicit StatusReporter(
      std::chrono::milliseconds Interval, MainOpt &ApplicationMainOptions,
      std::unique_ptr<KafkaW::ProducerTopic> &ApplicationStatusProducerTopic,
      Streams &MainLoopStreams)
      : IO(), Period(Interval), AsioTimer(IO, Period), Running(false),
        MainOptions(ApplicationMainOptions), streams(MainLoopStreams),
        StatusProducerTopic(std::move(ApplicationStatusProducerTopic)) {
    this->start();
  }

  void reportStatus();

  ~StatusReporter();

private:
  void start();
  void run() { IO.run(); }
  asio::io_context IO;
  std::chrono::milliseconds Period;
  asio::steady_timer AsioTimer;
  std::atomic_bool Running;
  MainOpt &MainOptions;
  std::thread StatusThread;
  SharedLogger Logger = getLogger();
  Streams &streams;
  std::unique_ptr<KafkaW::ProducerTopic> StatusProducerTopic;
  /// Blocks until the timer thread has stopped
  void waitForStop();
};

} // namespace Forwarder
