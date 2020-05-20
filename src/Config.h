// SPDX-License-Identifier: BSD-2-Clause
//
// This code has been produced by the European Spallation Source
// and its partner institutes under the BSD 2 Clause License.
//
// See LICENSE.md at the top level for license information.
//
// Screaming Udder!                              https://esss.se

#pragma once

#include "CommandHandler.h"
#include "KafkaW/KafkaW.h"
#include "URI.h"
#include <atomic>
#include <chrono>
#include <memory>
#include <string>
#include <vector>

namespace KafkaW {
class ConsumerInterface;
}

namespace Forwarder {
class ConfigCallback;
namespace Config {

class Listener {
public:
  Listener(URI uri, std::unique_ptr<KafkaW::ConsumerInterface> NewConsumer);
  Listener(Listener const &) = delete;
  ~Listener() = default;
  void poll(::Forwarder::ConfigCallback &cb);

private:
  std::unique_ptr<KafkaW::ConsumerInterface> Consumer;
  SharedLogger Logger = getLogger();
};
} // namespace Config
} // namespace Forwarder
