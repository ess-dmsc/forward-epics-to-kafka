// SPDX-License-Identifier: BSD-2-Clause
//
// This code has been produced by the European Spallation Source
// and its partner institutes under the BSD 2 Clause License.
//
// See LICENSE.md at the top level for license information.
//
// Screaming Udder!                              https://esss.se

#include "StatusTimer.h"
#include "json.h"

namespace Forwarder {

void StatusTimer::start() {
  Logger->trace("Starting the StatusTimer");
  Running = true;
  AsioTimer.async_wait(
      [this](std::error_code const & /*error*/) { this->reportStatus(); });
  StatusThread = std::thread(&StatusTimer::run, this);
}

void StatusTimer::waitForStop() {
  Logger->trace("Stopping StatusTimer");
  Running = false;
  AsioTimer.cancel();
  StatusThread.join();
}

void StatusTimer::reportStatus() {
  using nlohmann::json;
  auto Status = json::object();
  Status["service_id"] = MainOptions.MainSettings.ServiceID;
  auto Streams = json::array();
  auto StreamVector = streams.getStreams();
  std::transform(StreamVector.cbegin(), StreamVector.cend(),
                 std::back_inserter(Streams),
                 [](const std::shared_ptr<Stream> &CStream) {
                   return CStream->getStatusJson();
                 });
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
  status_producer_topic->produce((unsigned char *)StatusString.c_str(),
                                 StatusString.size());
  AsioTimer.expires_at(AsioTimer.expires_at() + Period);
  AsioTimer.async_wait(
      [this](std::error_code const & /*error*/) { this->reportStatus(); });
}

StatusTimer::~StatusTimer() { this->waitForStop(); }
} // namespace Forwarder