#pragma once
#include <memory>
#include "fbschemas.h"
#include "KafkaW.h"

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

private:
  KafkaW::Producer::Topic pt;
};
}
}
