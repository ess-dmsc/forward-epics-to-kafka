#include "MockKafkaInstanceSet.h"
#include <MetricsReporter.h>
#include <gtest/gtest.h>
#include <trompeloeil.hpp>

using namespace std::chrono_literals;
using trompeloeil::_;

class MetricsReporterTest : public ::testing::Test {};

namespace Forwarder {

void waitForReportIterations(MetricsReporter &Reporter,
                             uint32_t const NumberOfIterationsToWaitFor) {
  auto PollInterval = 50ms;
  auto TotalWait = 0ms;
  auto TimeOut = 10s; // Max wait time, prevent test hanging indefinitely
  while (Reporter.ReportIterations < NumberOfIterationsToWaitFor &&
         TotalWait < TimeOut) {
    std::this_thread::sleep_for(PollInterval);
    TotalWait += PollInterval;
  }
}

TEST(MetricsReporterTest, MetricsReporterLogsKafkaMetrics) {
  auto Interval = 10ms;
  auto TestKafkaInstanceSet = std::shared_ptr<InstanceSet>(
      new MockKafkaInstanceSet(KafkaW::BrokerSettings()));
  auto KafkaInstanceSet =
      dynamic_cast<MockKafkaInstanceSet *>(TestKafkaInstanceSet.get());
  MainOpt MainOptions;
  MetricsReporter TestMetricsTimer(Interval, MainOptions, TestKafkaInstanceSet);

  uint32_t const ReportsToWaitFor = 2;

  REQUIRE_CALL(*KafkaInstanceSet, logMetrics())
      .TIMES(AT_LEAST(ReportsToWaitFor));

  waitForReportIterations(TestMetricsTimer, ReportsToWaitFor);
}
} // namespace Forwarder
