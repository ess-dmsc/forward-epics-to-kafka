#include "Config.h"
#include "logger.h"
#include <condition_variable>
#include <memory>
#include <mutex>
#include <iostream>

namespace BrightnESS {
namespace ForwardEpicsToKafka {
namespace Config {

Listener::Listener(std::unique_ptr<KafkaW::BaseConsumer> baseConsumer) {
  impl.reset(new Listener_impl);
  impl->consumer.swap(baseConsumer);
  auto &consumer = *impl->consumer;
  consumer.on_rebalance_assign =
      [this](rd_kafka_topic_partition_list_t *plist) {
        {
          std::unique_lock<std::mutex> lock(impl->mx);
          impl->connected = 1;
        }
        impl->cv.notify_all();
      };
}

Listener::~Listener() {}

void Listener::poll(Callback &cb) {
  if (auto m = impl->consumer->poll().is_Msg()) {
    cb({(char *)m->data(), m->size()});
  }
}

void Listener::wait_for_connected(std::chrono::milliseconds timeout) {
  std::unique_lock<std::mutex> lock(impl->mx);
  impl->cv.wait_for(lock, timeout, [this] { return impl->connected == 1; });
}
}
}
}
