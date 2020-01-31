#include "MockProducer.h"
#include <StatusReporter.h>
#include <gtest/gtest.h>
#include <trompeloeil.hpp>

using namespace std::chrono_literals;
using trompeloeil::_;

class StatusReporterTest : public ::testing::Test {};

namespace Forwarder {
TEST(StatusReporterTest, StatusReporterCallsProduce) {
  const uint MinExpectedCallCount = 1;
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

  REQUIRE_CALL(*MockKafkaProducer, produce(_, _, _, _, _, _, _, _))
      .TIMES(AT_LEAST(MinExpectedCallCount))
      .RETURN(RdKafka::ErrorCode::ERR_NO_ERROR);

  StatusReporter TestStatusReporter(Interval, MainOptions,
                                    ApplicationStatusProducerTopic, streams);

  int CallCountTimeoutSeconds = 5;
  using clock = std::chrono::high_resolution_clock;
  using time_point = std::chrono::high_resolution_clock::time_point;
  time_point Tic;
  time_point Toc;
  Tic = clock::now();

  while (TestStatusReporter.getReportStatusCallCount() < MinExpectedCallCount) {
    Toc = clock::now();
    if (std::chrono::duration_cast<std::chrono::seconds>((Toc - Tic)).count() >=
        CallCountTimeoutSeconds)
      break;
  }
}

} // namespace Forwarder
