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

  ALLOW_CALL(*MockKafkaProducer, getRdKafkaPtr())
      .RETURN(ANY(RdKafka::Producer *));

  REQUIRE_CALL(*MockKafkaProducer,
               produce(ANY(RdKafka::Topic *), ANY(int32_t), ANY(int),
                       ANY(void *), ANY(size_t), ANY(const void *), ANY(size_t),
                       ANY(void *)))
      .TIMES(AT_LEAST(1))
      .RETURN(ANY(RdKafka::ErrorCode));

  std::this_thread::sleep_for(100ms);
}

} // namespace Forwarder
