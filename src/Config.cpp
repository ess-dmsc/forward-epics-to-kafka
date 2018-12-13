#include "Config.h"
#include "KafkaW/MetadataException.h"
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
  try {
    consumer.addTopic(uri.topic);
  } catch (MetadataException &E) {
    LOG(Sev::Error, "{}", E.what());
  }
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
