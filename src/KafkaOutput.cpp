#include "KafkaOutput.h"
#include "Forwarder.h"
#include "logger.h"

namespace Forwarder {

KafkaOutput::KafkaOutput(KafkaOutput &&x) noexcept
    : Output(std::move(x.Output)) {}

KafkaOutput::KafkaOutput(KafkaW::ProducerTopic &&OutputTopic)
    : Output(std::move(OutputTopic)) {}

int KafkaOutput::emit(std::unique_ptr<FlatBufs::FlatbufferMessage> fb) {
  if (!fb) {
    spdlog::get("ForwarderLogger")->trace("KafkaOutput::emit  empty fb");
    return -1024;
  }
  auto m1 = fb->message();
  fb->data = m1.data;
  fb->size = m1.size;
  std::unique_ptr<KafkaW::ProducerMessage> msg(fb.release());
  auto x = Output.produce(msg);
  if (x == 0) {
    ++g__total_msgs_to_kafka;
    g__total_bytes_to_kafka += m1.size;
  }
  return x;
}

std::string KafkaOutput::topic_name() { return Output.name(); }
} // namespace Forwarder
