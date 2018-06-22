#pragma once

#include "KafkaW/KafkaW.h"
#include "uri.h"
#include <atomic>
#include <chrono>
#include <memory>
#include <string>
#include <vector>

namespace Forwarder {

class Remote_T;

namespace Config {

using std::string;

/** Interface to react on configuration messages */
class Callback {
public:
  virtual void operator()(string const &msg) = 0;
};

struct Listener_impl;

class Listener {
public:
  Listener(KafkaW::BrokerSettings bopt, URI uri);
  Listener(Listener const &) = delete;
  ~Listener();
  void poll(Callback &cb);
  void wait_for_connected(std::chrono::milliseconds timeout);

private:
  std::unique_ptr<Listener_impl> impl;
  friend class Remote_T;
};
}
}
