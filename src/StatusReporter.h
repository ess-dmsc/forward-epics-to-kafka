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
  StatusReporter(
      std::chrono::milliseconds Interval, MainOpt &ApplicationMainOptions,
      std::unique_ptr<KafkaW::ProducerTopic> &ApplicationStatusProducerTopic,
      Streams &MainLoopStreams);

  void reportStatus();
  ~StatusReporter();

  // Used for testing, so that we can poll to see when report iterations have
  // happened instead of relying on them happening within a short time frame
  std::atomic_uint64_t ReportIterations{0};

private:
  void run() { IO.run(); }
  asio::io_context IO;
  std::chrono::milliseconds Period;
  asio::steady_timer AsioTimer;
  MainOpt &MainOptions;
  std::thread StatusThread;
  SharedLogger Logger = getLogger();
  Streams &streams;
  std::unique_ptr<KafkaW::ProducerTopic> StatusProducerTopic;
};

} // namespace Forwarder
