#include "MockProducer.h"
#include "ReporterHelpers.h"
#include "StatusReporter.h"
#include <gtest/gtest.h>
#include <trompeloeil.hpp>

using namespace std::chrono_literals;
using trompeloeil::_;

class StatusReporterTest : public ::testing::Test {};

namespace Forwarder {

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
  std::atomic<uint32_t> ReportIterations{0};

  REQUIRE_CALL(*MockKafkaProducer, produce(_, _, _, _, _, _, _, _))
      .TIMES(AT_LEAST(ReportsToWaitFor))
      .RETURN(RdKafka::ErrorCode::ERR_NO_ERROR)
      .LR_SIDE_EFFECT(ReportIterations++);
  ;

  StatusReporter TestStatusReporter(Interval, MainOptions,
                                    ApplicationStatusProducerTopic, streams);

  waitForReportIterations(ReportsToWaitFor, ReportIterations);
}

} // namespace Forwarder
