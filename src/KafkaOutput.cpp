#include "KafkaOutput.h"
#include "Forwarder.h"
#include "logger.h"

namespace Forwarder {

KafkaOutput::KafkaOutput(KafkaOutput &&x) : pt(std::move(x.pt)) {}

KafkaOutput::KafkaOutput(KafkaW::Producer::Topic &&pt) : pt(std::move(pt)) {}

int KafkaOutput::emit(FlatBufs::FlatbufferMessage::uptr fb) {
  if (!fb) {
    CLOG(8, 1, "KafkaOutput::emit  empty fb");
    return -1024;
  }
  auto m1 = fb->message();
  fb->data = m1.data;
  fb->size = m1.size;
  std::unique_ptr<KafkaW::Producer::Msg> msg(fb.release());
  auto x = pt.produce(msg);
  if (x == 0) {
    ++g__total_msgs_to_kafka;
    g__total_bytes_to_kafka += m1.size;
  }
  return x;
}

std::string KafkaOutput::topic_name() { return pt.name(); }
}
