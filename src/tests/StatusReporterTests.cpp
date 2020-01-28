#include "MockProducer.h"
#include <StatusReporter.h>
#include <gtest/gtest.h>
#include <trompeloeil.hpp>

using namespace std::chrono_literals;
using trompeloeil::_;

class StatusReporterTest : public ::testing::Test {};

namespace Forwarder {
TEST(StatusReporterTest, StatusReporterCallsProduce) {
  std::chrono::milliseconds Interval(10);
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
      .TIMES(AT_LEAST(1))
      .RETURN(RdKafka::ErrorCode::ERR_NO_ERROR);

  // These braces ensure reporter is stopped and cleaned up before the mock
  // expectation is, otherwise we occasionally see the test fail with an
  // unmatched call of produce() on the mock
  {
    StatusReporter TestStatusReporter(Interval, MainOptions,
                                      ApplicationStatusProducerTopic, streams);
    std::this_thread::sleep_for(100ms);
  }
}

} // namespace Forwarder
