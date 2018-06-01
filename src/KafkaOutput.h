#pragma once

#include "FlatbufferMessage.h"
#include "KafkaW/KafkaW.h"
#include <memory>

namespace Forwarder {

/**
Represents the output sink used by Stream.
*/
class KafkaOutput {
public:
  KafkaOutput(KafkaOutput &&pt);
  KafkaOutput(KafkaW::Producer::Topic &&pt);
  /// Hands off the message to Kafka
  int emit(std::unique_ptr<FlatBufs::FlatbufferMessage> fb);
  std::string topic_name();
  KafkaW::Producer::Topic pt;
};
}
