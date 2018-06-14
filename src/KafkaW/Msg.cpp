#include "Msg.h"

#include <librdkafka/rdkafka.h>

namespace KafkaW {

Msg::~Msg() { rd_kafka_message_destroy((rd_kafka_message_t *)MsgPtr); }

uchar *Msg::data() const {
  return (uchar *)((rd_kafka_message_t *)MsgPtr)->payload;
}

size_t Msg::size() const { return ((rd_kafka_message_t *)MsgPtr)->len; }

} // namespace KafkaW
