#pragma once

#include "CommandHandler.h"
#include "KafkaW/Consumer.h"
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
class ConfigCB;
namespace Config {

class Listener {
public:
  Listener(URI uri, std::unique_ptr<KafkaW::ConsumerInterface> NewConsumer);
  Listener(Listener const &) = delete;
  ~Listener() = default;
  void poll(::Forwarder::ConfigCB &cb);

private:
  std::unique_ptr<KafkaW::ConsumerInterface> Consumer;
  std::shared_ptr<spdlog::logger> Logger = spdlog::get("ForwarderLogger");
};
} // namespace Config
} // namespace Forwarder
