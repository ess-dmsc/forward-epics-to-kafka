// SPDX-License-Identifier: BSD-2-Clause
//
// This code has been produced by the European Spallation Source
// and its partner institutes under the BSD 2 Clause License.
//
// See LICENSE.md at the top level for license information.
//
// Screaming Udder!                              https://esss.se

#include "StatusReporter.h"
#include "json.h"

namespace Forwarder {

StatusReporter::StatusReporter(
    std::chrono::milliseconds Interval, MainOpt &ApplicationMainOptions,
    std::unique_ptr<KafkaW::ProducerTopic> &ApplicationStatusProducerTopic,
    Streams &MainLoopStreams)
    : IO(), Period(Interval), AsioTimer(IO, Period),
      MainOptions(ApplicationMainOptions), streams(MainLoopStreams),
      StatusProducerTopic(std::move(ApplicationStatusProducerTopic)) {
  Logger->trace("Starting the StatusTimer");
  AsioTimer.async_wait([this](std::error_code const &Error) {
    if (Error != asio::error::operation_aborted) {
      this->reportStatus();
    }
  });
  StatusThread = std::thread(&StatusReporter::run, this);
}

void StatusReporter::reportStatus() {
  if (!StatusProducerTopic) {
    return;
  }
  using nlohmann::json;
  auto Status = json::object();
  Status["service_id"] = MainOptions.MainSettings.ServiceID;
  auto Streams = streams.getStreamStatuses();
  Status["streams"] = Streams;
  auto StatusString = Status.dump();
  auto StatusStringSize = StatusString.size();
  if (StatusStringSize > 1000) {
    auto StatusStringShort =
        StatusString.substr(0, 1000) +
        fmt::format(" ... {} chars total ...", StatusStringSize);
    Logger->debug("status: {}", StatusStringShort);
  } else {
    Logger->debug("status: {}", StatusString);
  }
  StatusProducerTopic->produce((unsigned char *)StatusString.c_str(),
                               StatusString.size());

  AsioTimer.expires_at(AsioTimer.expires_at() + Period);
  AsioTimer.async_wait([this](std::error_code const &Error) {
    if (Error != asio::error::operation_aborted) {
      this->reportStatus();
    }
  });
}

StatusReporter::~StatusReporter() {
  Logger->trace("Stopping StatusTimer");
  IO.stop();
  StatusThread.join();
}
} // namespace Forwarder
