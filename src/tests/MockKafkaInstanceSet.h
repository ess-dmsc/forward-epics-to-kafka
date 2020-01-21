#pragma once

#include <Kafka.h>
#include <trompeloeil.hpp>

namespace Forwarder {
class MockKafkaInstanceSet : public InstanceSet {
public:
  using InstanceSet::InstanceSet;
  MAKE_MOCK0(logMetrics, void(), override);
};
} // namespace Forwarder
