#include "MockKafkaInstanceSet.h"
#include <MetricsReporter.h>
#include <gtest/gtest.h>
#include <trompeloeil.hpp>

using namespace std::chrono_literals;
using trompeloeil::_;

class MetricsReporterTest : public ::testing::Test {};

namespace Forwarder {
TEST(MetricsReporterTest, MetricsReporterLogsKafkaMetrics) {
  const uint MinExpectedCallCount = 2;
  auto Interval = 10ms;
  auto TestKafkaInstanceSet = std::shared_ptr<InstanceSet>(
      new MockKafkaInstanceSet(KafkaW::BrokerSettings()));
  auto KafkaInstanceSet =
      dynamic_cast<MockKafkaInstanceSet *>(TestKafkaInstanceSet.get());
  MainOpt MainOptions;
  MetricsReporter TestMetricsTimer(Interval, MainOptions, TestKafkaInstanceSet);

  REQUIRE_CALL(*KafkaInstanceSet, logMetrics())
      .TIMES(AT_LEAST(MinExpectedCallCount));

  int CallCountTimeoutSeconds = 5;
  using clock = std::chrono::high_resolution_clock;
  using time_point = std::chrono::high_resolution_clock::time_point;
  time_point Tic;
  time_point Toc;
  Tic = clock::now();

  while (TestMetricsTimer.getReportMetricsCallCount() < MinExpectedCallCount) {
    Toc = clock::now();
    if (std::chrono::duration_cast<std::chrono::seconds>((Toc - Tic)).count() >=
        CallCountTimeoutSeconds)
      break;
  }
}

} // namespace Forwarder
