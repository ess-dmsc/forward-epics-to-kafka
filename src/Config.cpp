#include "Config.h"
#include "logger.h"
#include <condition_variable>
#include <memory>
#include <mutex>

namespace Forwarder {
namespace Config {

struct Listener_impl {
  std::unique_ptr<KafkaW::ConsumerInterface> consumer;
  std::mutex mx;
  std::condition_variable cv;
  int connected = 0;
};

Listener::Listener(URI uri,
                   std::unique_ptr<KafkaW::ConsumerInterface> NewConsumer) {
  impl.reset(new Listener_impl);
  impl->consumer = std::move(NewConsumer);
  auto &consumer = *impl->consumer;
  //  consumer.on_rebalance_assign =
  //      [this](rd_kafka_topic_partition_list_t *plist) {
  //        UNUSED_ARG(plist);
  //        {
  //          std::unique_lock<std::mutex> lock(impl->mx);
  //          impl->connected = 1;
  //        }
  //        impl->cv.notify_all();
  //      };
  //  consumer.on_rebalance_assign = {};
  //  consumer.on_rebalance_start = {};
  consumer.addTopic(uri.topic);
}

void Listener::poll(Callback &cb) {
  auto Message = impl->consumer->poll();
  if (Message->getStatus() == KafkaW::PollStatus::Msg) {
    cb({(char *)Message->getData(), Message->getSize()});
  }
}

Listener::~Listener() {}
} // namespace Config
} // namespace Forwarder
