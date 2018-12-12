#pragma once

#include "CommandHandler.h"
#include "KafkaW/KafkaW.h"
#include "uri.h"
#include <atomic>
#include <chrono>
#include <memory>
#include <string>
#include <vector>

namespace Forwarder {
class ConfigCB;

namespace Config {

using std::string;

struct Listener_impl;

class Listener {
public:
  Listener(KafkaW::BrokerSettings bopt, URI uri);
  Listener(Listener const &) = delete;
  ~Listener();
  void poll(::Forwarder::ConfigCB &cb);

private:
  std::unique_ptr<Listener_impl> impl;
};
} // namespace Config
} // namespace Forwarder
