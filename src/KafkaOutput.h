#pragma once

#include "FlatbufferMessage.h"
#include "KafkaW/ProducerTopic.h"
#include <memory>

namespace Forwarder {

/// Represents the output sink used by Stream.
class KafkaOutput {
public:
  KafkaOutput(KafkaOutput &&) noexcept;
  explicit KafkaOutput(KafkaW::ProducerTopic &&OutputTopic);
  /// Hands off the message to Kafka
  int emit(std::unique_ptr<FlatBufs::FlatbufferMessage> fb);
  std::string topic_name();
  KafkaW::ProducerTopic Output;
};
} // namespace Forwarder
