#pragma once

#include "KafkaW.h"
#include "uri.h"
#include "helper.h"
#include <atomic>
#include <chrono>
#include <memory>
#include <string>
#include <vector>
#include <condition_variable>

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

struct Listener_impl{
  std::unique_ptr<KafkaW::BaseConsumer> consumer;
  std::mutex mx;
  std::condition_variable cv;
  int connected = 0;
};

class Listener {
public:
  Listener(std::unique_ptr<KafkaW::BaseConsumer> baseConsumer);
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
