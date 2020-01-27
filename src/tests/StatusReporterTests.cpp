#include "MockProducer.h"
#include <StatusReporter.h>
#include <gtest/gtest.h>
#include <trompeloeil.hpp>

using namespace std::chrono_literals;
using trompeloeil::_;

class StatusReporterTest : public ::testing::Test {};

namespace Forwarder {
TEST(StatusReporterTest, StatusReporterDoesSomething) {
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

  StatusReporter TestStatusReporter(Interval, MainOptions,
                                    ApplicationStatusProducerTopic, streams);

  REQUIRE_CALL(*MockKafkaProducer, produce(_, _, _, _, _, _, _, _))
      .TIMES(AT_LEAST(1))
      .RETURN(RdKafka::ErrorCode::ERR_NO_ERROR);

  std::this_thread::sleep_for(100ms);
}

} // namespace Forwarder
