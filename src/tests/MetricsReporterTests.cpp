#include "MetricsReporter.h"
#include "MockKafkaInstanceSet.h"
#include "ReporterHelpers.h"
#include <gtest/gtest.h>
#include <trompeloeil.hpp>

using namespace std::chrono_literals;
using trompeloeil::_;

class MetricsReporterTest : public ::testing::Test {};

namespace Forwarder {

TEST(MetricsReporterTest, MetricsReporterLogsKafkaMetrics) {
  auto Interval = 10ms;
  auto TestKafkaInstanceSet = std::shared_ptr<InstanceSet>(
      new MockKafkaInstanceSet(KafkaW::BrokerSettings()));
  auto KafkaInstanceSet =
      dynamic_cast<MockKafkaInstanceSet *>(TestKafkaInstanceSet.get());
  MainOpt MainOptions;
  MetricsReporter TestMetricsTimer(Interval, MainOptions, TestKafkaInstanceSet);

  uint32_t const ReportsToWaitFor = 2;
  std::atomic<uint32_t> ReportIterations{0};

  REQUIRE_CALL(*KafkaInstanceSet, logMetrics())
      .TIMES(AT_LEAST(ReportsToWaitFor))
      .LR_SIDE_EFFECT(ReportIterations++);

  waitForReportIterations(ReportsToWaitFor, ReportIterations);
}
} // namespace Forwarder
