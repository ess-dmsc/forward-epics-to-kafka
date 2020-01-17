#include "MockKafkaInstanceSet.h"
#include <MetricsTimer.h>
#include <gtest/gtest.h>
#include <trompeloeil.hpp>

using trompeloeil::_;

class MetricsTimerTest : public ::testing::Test {};

namespace Forwarder {
TEST(MetricsTimerTest, BasicTest) {
  std::chrono::milliseconds Interval(100);
  std::shared_ptr<InstanceSet> TestKafkaInstanceSet =
      std::unique_ptr<InstanceSet>(
          new MockKafkaInstanceSet(KafkaW::BrokerSettings()));
  MainOpt MainOptions;
  MetricsTimer TestMetricsTimer(Interval, MainOptions, TestKafkaInstanceSet);
}

} // namespace Forwarder