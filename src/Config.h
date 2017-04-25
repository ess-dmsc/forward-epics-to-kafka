#pragma once

#include <memory>
#include <atomic>
#include <chrono>
#include <vector>
#include <string>
#include "uri.h"
#include "KafkaW.h"

namespace BrightnESS {
namespace ForwardEpicsToKafka {

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
  Listener(KafkaW::BrokerOpt bopt, uri::URI uri);
  Listener(Listener const &) = delete;
  ~Listener();
  void poll(Callback &cb);
  void wait_for_connected(std::chrono::milliseconds timeout);

private:
  std::unique_ptr<Listener_impl> impl;
  friend class ForwardEpicsToKafka::Remote_T;
};
}
}
}
