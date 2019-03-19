#include "Config.h"
#include "KafkaW/KafkaW.h"
#include "KafkaW/MetadataException.h"
#include "logger.h"
#include <condition_variable>
#include <memory>
#include <mutex>

namespace Forwarder {
namespace Config {

Listener::Listener(URI uri,
                   std::unique_ptr<KafkaW::ConsumerInterface> NewConsumer)
    : Consumer(std::move(NewConsumer)) {
  try {
    Consumer->addTopic(uri.Topic);
  } catch (MetadataException &E) {
    LOG(Sev::Error, "{}", E.what());
  }
}

void Listener::poll(::Forwarder::ConfigCB &cb) {
  auto Message = Consumer->poll();
  if (Message->getStatus() == KafkaW::PollStatus::Message) {
    cb(Message->getData());
  }
}
} // namespace Config
} // namespace Forwarder
