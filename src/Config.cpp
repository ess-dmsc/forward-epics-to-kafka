#include "Config.h"
#include "logger.h"
#include <condition_variable>
#include <memory>
#include <mutex>

namespace Forwarder {
namespace Config {

struct Listener_impl {
  std::unique_ptr<KafkaW::Consumer> consumer;
  std::mutex mx;
  std::condition_variable cv;
  int connected = 0;
};

Listener::Listener(KafkaW::BrokerSettings BrokerSettings, URI Uri) {
  BrokerSettings.Address = Uri.host_port;
  BrokerSettings.PollTimeoutMS = 0;
  impl.reset(new Listener_impl);
  impl->consumer.reset(new KafkaW::Consumer(BrokerSettings));
  auto &consumer = *impl->consumer;
  consumer.on_rebalance_assign = [this](rd_kafka_topic_partition_list_t *) {
    {
      std::unique_lock<std::mutex> lock(impl->mx);
      impl->connected = 1;
    }
    impl->cv.notify_all();
  };
  consumer.on_rebalance_assign = {};
  consumer.on_rebalance_start = {};
  consumer.addTopic(Uri.topic);
}

Listener::~Listener() {}

void Listener::poll(::Forwarder::ConfigCB &cb) {
  auto Message = impl->consumer->poll();
  if (Message->getStatus() == KafkaW::PollStatus::Msg) {
    cb({(char *)Message->getData(), Message->getSize()});
  }
}
} // namespace Config
} // namespace Forwarder
