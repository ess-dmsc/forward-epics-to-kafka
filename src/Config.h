#pragma once

#include "KafkaW/KafkaW.h"
#include "uri.h"
#include <atomic>
#include <chrono>
#include <memory>
#include <string>
#include <vector>

namespace Forwarder {

namespace Config {

using std::string;

/// Interface to react on configuration messages.
class Callback {
public:
  virtual void operator()(string const &msg) = 0;
};

struct Listener_impl;

class Listener {
public:
  Listener(URI uri, std::unique_ptr<KafkaW::ConsumerInterface> NewConsumer);
  Listener(Listener const &) = delete;
  ~Listener();
  void poll(Callback &cb);

private:
  std::unique_ptr<Listener_impl> impl;
};
} // namespace Config
} // namespace Forwarder
