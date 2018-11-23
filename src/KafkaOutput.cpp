#include "KafkaOutput.h"
#include "Forwarder.h"
#include "logger.h"

namespace Forwarder {

KafkaOutput::KafkaOutput(KafkaOutput &&x) noexcept : pt(std::move(x.pt)) {}

KafkaOutput::KafkaOutput(KafkaW::Producer::Topic &&pt) : pt(std::move(pt)) {}

int KafkaOutput::emit(std::unique_ptr<FlatBufs::FlatbufferMessage> fb) {
  if (!fb) {
    LOG(Sev::Debug, "KafkaOutput::emit  empty fb");
    return -1024;
  }
  auto m1 = fb->message();
  fb->data = m1.first;
  fb->size = m1.second;
  std::unique_ptr<KafkaW::Producer::Msg> msg(fb.release());
  auto x = pt.produce(msg);
  if (x == 0) {
    ++g__total_msgs_to_kafka;
    g__total_bytes_to_kafka += m1.second;
  }
  return x;
}

std::string KafkaOutput::topic_name() { return pt.name(); }
} // namespace Forwarder
