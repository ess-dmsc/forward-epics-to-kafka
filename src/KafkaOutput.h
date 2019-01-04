#pragma once

#include "FlatbufferMessage.h"
#include "KafkaW/KafkaW.h"
#include <memory>

namespace Forwarder {

/// Represents the output sink used by Stream.
class KafkaOutput {
public:
  KafkaOutput(KafkaOutput &&) noexcept;
  explicit KafkaOutput(KafkaW::Producer::Topic &&OutputTopic);
  /// Hands off the message to Kafka
  int emit(std::unique_ptr<FlatBufs::FlatbufferMessage> fb);
  std::string topic_name();
  KafkaW::Producer::Topic Output;
};
} // namespace Forwarder
