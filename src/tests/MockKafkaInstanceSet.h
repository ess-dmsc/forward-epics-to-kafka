#pragma once

#include <Kafka.h>
#include <trompeloeil.hpp>

namespace Forwarder {
class MockKafkaInstanceSet : public InstanceSet {
public:
  using InstanceSet::InstanceSet;
  //  explicit MockKafkaInstanceSet(KafkaW::BrokerSettings brokerSettings)
  //      : InstanceSet(brokerSettings){};
  MAKE_MOCK0(logMetrics, void());
};
} // namespace Forwarder