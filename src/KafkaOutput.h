#pragma once
#include "KafkaW.h"
#include "fbschemas.h"
#include <memory>

namespace BrightnESS {
namespace ForwardEpicsToKafka {

/**
Represents the output sink used by Stream.
*/
class KafkaOutput {
public:
  KafkaOutput(KafkaOutput &&pt);
  KafkaOutput(KafkaW::Producer::Topic &&pt);
  /// Hands off the message to Kafka
  int emit(std::unique_ptr<BrightnESS::FlatBufs::FB> fb);
  std::string topic_name();
  KafkaW::Producer::Topic pt;
};
}
}
