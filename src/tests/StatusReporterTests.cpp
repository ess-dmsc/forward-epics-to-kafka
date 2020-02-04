#include "MockProducer.h"
#include <StatusReporter.h>
#include <gtest/gtest.h>
#include <trompeloeil.hpp>

using namespace std::chrono_literals;
using trompeloeil::_;

class StatusReporterTest : public ::testing::Test {};

namespace Forwarder {

void waitForReportIterations(StatusReporter &Reporter,
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

TEST(StatusReporterTest, StatusReporterCallsProduce) {
  auto Interval = 10ms;
  MainOpt MainOptions;
  std::string TopicName = "Topic Name";
  KafkaW::BrokerSettings Settings;
  auto KafkaProducer =
      std::shared_ptr<KafkaW::Producer>(new KafkaW::MockProducer(Settings));
  auto MockKafkaProducer =
      dynamic_cast<KafkaW::MockProducer *>(KafkaProducer.get());
  Streams streams;
  std::unique_ptr<KafkaW::ProducerTopic> ApplicationStatusProducerTopic;
  ApplicationStatusProducerTopic =
      std::make_unique<KafkaW::ProducerTopic>(KafkaProducer, TopicName);

  uint32_t const ReportsToWaitFor = 2;

  REQUIRE_CALL(*MockKafkaProducer, produce(_, _, _, _, _, _, _, _))
      .TIMES(AT_LEAST(ReportsToWaitFor))
      .RETURN(RdKafka::ErrorCode::ERR_NO_ERROR);

  StatusReporter TestStatusReporter(Interval, MainOptions,
                                    ApplicationStatusProducerTopic, streams);

  waitForReportIterations(TestStatusReporter, ReportsToWaitFor);
}

} // namespace Forwarder
