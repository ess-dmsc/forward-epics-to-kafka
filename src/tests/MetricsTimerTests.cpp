#include "MockKafkaInstanceSet.h"
#include <MetricsTimer.h>
#include <gtest/gtest.h>
#include <trompeloeil.hpp>

using namespace std::chrono_literals;
using trompeloeil::_;

class MetricsTimerTest : public ::testing::Test {};

namespace Forwarder {
TEST(MetricsTimerTest, MetricsTimerLogsKafkaMetrics) {
  std::chrono::milliseconds Interval(10);
  auto TestKafkaInstanceSet = std::shared_ptr<InstanceSet>(
      new MockKafkaInstanceSet(KafkaW::BrokerSettings()));
  auto KafkaInstanceSet =
      dynamic_cast<MockKafkaInstanceSet *>(TestKafkaInstanceSet.get());
  MainOpt MainOptions;
  MetricsTimer TestMetricsTimer(Interval, MainOptions, TestKafkaInstanceSet);

  REQUIRE_CALL(*KafkaInstanceSet, logMetrics()).TIMES(AT_LEAST(2));

  std::this_thread::sleep_for(100ms);
}

} // namespace Forwarder
