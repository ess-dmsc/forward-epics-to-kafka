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
    Consumer->addTopic(uri.topic);
  } catch (MetadataException &E) {
    LOG(Sev::Error, "{}", E.what());
  }
}

void Listener::poll(::Forwarder::ConfigCB &cb) {
  auto Message = Consumer->poll();
  if (Message->getStatus() == KafkaW::PollStatus::Msg) {
    cb({reinterpret_cast<const char *>(Message->getData()),
        Message->getSize()});
  }
}
} // namespace Config
} // namespace Forwarder
