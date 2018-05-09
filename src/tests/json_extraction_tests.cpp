#include "../Converter.h"
#include "../Main.h"
#include "../MainOpt.h"
#include "../configuration.h"
#include "../helper.h"
#include <gtest/gtest.h>
#include <iostream>
#include <sstream>
#include <string>

TEST(json_extraction_tests, no_converters_specified_has_no_side_effects) {
  std::string RawJson = "{}";

  std::string schema("f142");

  BrightnESS::ForwardEpicsToKafka::MainOpt Main;
  Main.Config->setJsonFromString(RawJson);

  Main.Config->extractGlobalConverters(schema);

  ASSERT_EQ(0u, Main.Config->ConverterInts.size());
  ASSERT_EQ(0u, Main.Config->ConverterStrings.size());
}

TEST(json_extraction_tests, ints_specified_in_converters_is_extracted) {
  std::string RawJson = "{"
                        "  \"converters\": {"
                        "    \"f142\": { "
                        "      \"some_option1\": 123, "
                        "      \"some_option2\": 456"
                        "    }"
                        "  }"
                        "}";

  std::string schema("f142");

  BrightnESS::ForwardEpicsToKafka::MainOpt Main;
  Main.Config->setJsonFromString(RawJson);

  Main.Config->extractGlobalConverters(schema);

  ASSERT_EQ(2u, Main.Config->ConverterInts.size());
  ASSERT_EQ(123, Main.Config->ConverterInts["some_option1"]);
  ASSERT_EQ(456, Main.Config->ConverterInts["some_option2"]);
}

TEST(json_extraction_tests, strings_specified_in_converters_is_extracted) {
  std::string RawJson = "{"
                        "  \"converters\": {"
                        "    \"f142\": { "
                        "      \"some_option1\": \"hello\", "
                        "      \"some_option2\": \"goodbye\""
                        "    }"
                        "  }"
                        "}";

  std::string schema("f142");

  BrightnESS::ForwardEpicsToKafka::MainOpt Main;
  Main.Config->setJsonFromString(RawJson);

  Main.Config->extractGlobalConverters(schema);

  ASSERT_EQ(2u, Main.Config->ConverterStrings.size());
  ASSERT_EQ("hello", Main.Config->ConverterStrings["some_option1"]);
  ASSERT_EQ("goodbye", Main.Config->ConverterStrings["some_option2"]);
}

TEST(json_extraction_tests,
     extracting_status_uri_gives_correct_uri_port_and_topic) {
  std::string RawJson = "{"
                        "  \"status-uri\": \"//kafkabroker:1234/status_topic\""
                        "}";

  BrightnESS::ForwardEpicsToKafka::MainOpt Main;
  Main.Config->setJsonFromString(RawJson);

  Main.find_status_uri();

  ASSERT_EQ("kafkabroker", Main.StatusReportURI.host);
  ASSERT_EQ(1234u, Main.StatusReportURI.port);
  ASSERT_EQ("status_topic", Main.StatusReportURI.topic);
}

TEST(json_extraction_tests, no_status_uri_defined_gives_no_uri_port_or_topic) {
  std::string RawJson = "{}";

  BrightnESS::ForwardEpicsToKafka::MainOpt Main;
  Main.Config->setJsonFromString(RawJson);

  Main.find_status_uri();

  ASSERT_EQ("", Main.StatusReportURI.host);
  ASSERT_EQ(0u, Main.StatusReportURI.port);
  ASSERT_EQ("", Main.StatusReportURI.topic);
}

TEST(json_extraction_tests, setting_broker_sets_host_and_port) {
  std::string RawJson = "{"
                        "  \"broker\": \"kafkabroker:1234\""
                        "}";

  BrightnESS::ForwardEpicsToKafka::MainOpt Main;
  Main.Config->setJsonFromString(RawJson);

  Main.findBroker();

  ASSERT_EQ("kafkabroker", Main.brokers[0].host);
  ASSERT_EQ(1234u, Main.brokers[0].port);
}

TEST(json_extraction_tests,
     setting_multiple_brokers_sets_multiple_hosts_and_ports) {
  std::string RawJson = "{"
                        "  \"broker\": \"kafkabroker1:1234, kafkabroker2:5678\""
                        "}";

  BrightnESS::ForwardEpicsToKafka::MainOpt Main;
  Main.Config->setJsonFromString(RawJson);

  Main.findBroker();

  ASSERT_EQ(2u, Main.brokers.size());
  ASSERT_EQ("kafkabroker1", Main.brokers[0].host);
  ASSERT_EQ(1234u, Main.brokers[0].port);
  ASSERT_EQ("kafkabroker2", Main.brokers[1].host);
  ASSERT_EQ(5678u, Main.brokers[1].port);
}

TEST(json_extraction_tests, setting_no_brokers_sets_default_host_and_port) {
  std::string RawJson = "{}";

  BrightnESS::ForwardEpicsToKafka::MainOpt Main;
  Main.Config->setJsonFromString(RawJson);

  Main.findBroker();

  ASSERT_EQ("localhost", Main.brokers[0].host);
  ASSERT_EQ(9092u, Main.brokers[0].port);
}

TEST(json_extraction_tests, no_kafka_broker_settings_has_no_side_effects) {
  std::string RawJson = "{}";

  BrightnESS::ForwardEpicsToKafka::MainOpt Main;
  Main.Config->setJsonFromString(RawJson);

  Main.Config->extractKafkaBrokerSettings();

  BrightnESS::ForwardEpicsToKafka::KafkaBrokerSettings Settings = Main.Config->BrokerSettings;

  ASSERT_EQ(0u, Settings.ConfigurationIntegers.size());
  ASSERT_EQ(0u, Settings.ConfigurationStrings.size());
}

TEST(json_extraction_tests, ints_in_kafka_broker_settings_are_extracted) {
  std::string RawJson = "{"
                        "  \"kafka\": {"
                        "    \"broker\": { "
                        "      \"some_option1\": 123, "
                        "      \"some_option2\": 456"
                        "    }"
                        "  }"
                        "}";

  BrightnESS::ForwardEpicsToKafka::MainOpt Main;
  Main.Config->setJsonFromString(RawJson);

  Main.Config->extractKafkaBrokerSettings();

  BrightnESS::ForwardEpicsToKafka::KafkaBrokerSettings Settings = Main.Config->BrokerSettings;

  ASSERT_EQ(2u, Settings.ConfigurationIntegers.size());
  ASSERT_EQ(123, Settings.ConfigurationIntegers["some_option1"]);
  ASSERT_EQ(456, Settings.ConfigurationIntegers["some_option2"]);
}

TEST(json_extraction_tests, strings_in_kafka_broker_settings_are_extracted) {
  std::string RawJson = "{"
                        "  \"kafka\": {"
                        "    \"broker\": { "
                        "      \"some_option1\": \"hello\", "
                        "      \"some_option2\": \"goodbye\""
                        "    }"
                        "  }"
                        "}";

  BrightnESS::ForwardEpicsToKafka::MainOpt Main;
  Main.Config->setJsonFromString(RawJson);

  Main.Config->extractKafkaBrokerSettings();

  BrightnESS::ForwardEpicsToKafka::KafkaBrokerSettings Settings = Main.Config->BrokerSettings;

  ASSERT_EQ(2u, Settings.ConfigurationStrings.size());
  ASSERT_EQ("hello", Settings.ConfigurationStrings["some_option1"]);
  ASSERT_EQ("goodbye", Settings.ConfigurationStrings["some_option2"]);
}

TEST(json_extraction_tests,
     no_broker_config_settings_sets_default_host_port_and_topic) {
  std::string RawJson = "{}";

  BrightnESS::ForwardEpicsToKafka::MainOpt Main;
  Main.Config->setJsonFromString(RawJson);

  Main.findBrokerConfig();

  ASSERT_EQ("localhost", Main.BrokerConfig.host);
  ASSERT_EQ(9092u, Main.BrokerConfig.port);
  ASSERT_EQ("forward_epics_to_kafka_commands", Main.BrokerConfig.topic);
}

TEST(json_extraction_tests,
     extracting_broker_config_settings_sets_host_port_and_topic) {
  std::string RawJson =
      "{"
      "  \"broker-config\": \"//kafkabroker:1234/the_command_topic\""
      "}";

  BrightnESS::ForwardEpicsToKafka::MainOpt Main;
  Main.Config->setJsonFromString(RawJson);

  Main.findBrokerConfig();

  ASSERT_EQ("kafkabroker", Main.BrokerConfig.host);
  ASSERT_EQ(1234u, Main.BrokerConfig.port);
  ASSERT_EQ("the_command_topic", Main.BrokerConfig.topic);
}

TEST(json_extraction_tests, no_conversion_threads_settings_sets_default) {
  std::string RawJson = "{}";

  BrightnESS::ForwardEpicsToKafka::MainOpt Main;
  Main.Config->setJsonFromString(RawJson);

  Main.findConversionThreads();

  ASSERT_EQ(1u, Main.ConversionThreads);
}

TEST(json_extraction_tests, extracting_conversion_threads_sets_value) {
  std::string RawJson = "{"
                        "  \"conversion-threads\": 3"
                        "}";

  BrightnESS::ForwardEpicsToKafka::MainOpt Main;
  Main.Config->setJsonFromString(RawJson);

  Main.findConversionThreads();

  ASSERT_EQ(3u, Main.ConversionThreads);
}

TEST(json_extraction_tests, no_conversion_worker_queue_size_sets_default) {
  std::string RawJson = "{}";

  BrightnESS::ForwardEpicsToKafka::MainOpt Main;
  Main.Config->setJsonFromString(RawJson);

  Main.findConversionWorkerQueueSize();

  ASSERT_EQ(1024u, Main.ConversionWorkerQueueSize);
}

TEST(json_extraction_tests,
     extracting_conversion_worker_queue_size_sets_value) {
  std::string RawJson = "{"
                        "  \"conversion-worker-queue-size\": 1234"
                        "}";

  BrightnESS::ForwardEpicsToKafka::MainOpt Main;
  Main.Config->setJsonFromString(RawJson);

  Main.findConversionWorkerQueueSize();

  ASSERT_EQ(1234u, Main.ConversionWorkerQueueSize);
}

TEST(json_extraction_tests, no_main_poll_interval_sets_default) {
  std::string RawJson = "{}";

  BrightnESS::ForwardEpicsToKafka::MainOpt Main;
  Main.Config->setJsonFromString(RawJson);

  Main.findMainPollInterval();

  ASSERT_EQ(500, Main.main_poll_interval);
}

TEST(json_extraction_tests, extracting_main_poll_interval_sets_value) {
  std::string RawJson = "{"
                        "  \"main-poll-interval\": 1234"
                        "}";

  BrightnESS::ForwardEpicsToKafka::MainOpt Main;
  Main.Config->setJsonFromString(RawJson);

  Main.findMainPollInterval();

  ASSERT_EQ(1234, Main.main_poll_interval);
}

TEST(json_extraction_tests,
     extracting_converter_info_gets_schema_topic_and_name) {
  std::string RawJson = "{"
                        "  \"schema\": \"f142\", \"topic\": "
                        "\"Kafka_topic_name\", \"name\": \"my_name\""
                        "}";

  nlohmann::json Json = nlohmann::json::parse(RawJson);
  BrightnESS::ForwardEpicsToKafka::MainOpt MainOpt;
  BrightnESS::ForwardEpicsToKafka::Main Main(MainOpt);

  std::string Schema;
  std::string Topic;
  std::string Name;
  Main.extractConverterInfo(Json, Schema, Topic, Name);

  ASSERT_EQ("f142", Schema);
  ASSERT_EQ("Kafka_topic_name", Topic);
  ASSERT_EQ("my_name", Name);
}

//TEST(json_extraction_tests,
//     extracting_converter_info_with_no_name_gets_auto_named) {
//  std::string RawJson =
//      "{"
//      "  \"schema\": \"f142\", \"topic\": \"Kafka_topic_name\""
//      "}";
//
//  nlohmann::json Json = nlohmann::json::parse(RawJson);
//  BrightnESS::ForwardEpicsToKafka::MainOpt MainOpt;
//  BrightnESS::ForwardEpicsToKafka::Main Main(MainOpt);
//
//  std::string Schema;
//  std::string Topic;
//  std::string Name;
//  Main.extractConverterInfo(Json, Schema, Topic, Name);
//
//  // Don't carry what the name is, but it must be something
//  ASSERT_TRUE(!Name.empty());
//}
//
//TEST(json_extraction_tests, extracting_converter_info_with_no_schema_throws) {
//  std::string RawJson = "{"
//                        "  \"topic\": \"Kafka_topic_name\""
//                        "}";
//
//  nlohmann::json Json = nlohmann::json::parse(RawJson);
//  BrightnESS::ForwardEpicsToKafka::MainOpt MainOpt;
//  BrightnESS::ForwardEpicsToKafka::Main Main(MainOpt);
//
//  std::string Schema;
//  std::string Topic;
//  std::string Name;
//
//  ASSERT_ANY_THROW(Main.extractConverterInfo(Json, Schema, Topic, Name));
//}
//
//TEST(json_extraction_tests, extracting_converter_info_with_no_topic_throws) {
//  std::string RawJson = "{"
//                        "  \"schema\": \"f142\""
//                        "}";
//
//  nlohmann::json Json = nlohmann::json::parse(RawJson);
//  BrightnESS::ForwardEpicsToKafka::MainOpt MainOpt;
//  BrightnESS::ForwardEpicsToKafka::Main Main(MainOpt);
//
//  std::string Schema;
//  std::string Topic;
//  std::string Name;
//
//  ASSERT_ANY_THROW(Main.extractConverterInfo(Json, Schema, Topic, Name));
//}
//
//TEST(json_extraction_tests, extracting_mapping_info_gets_channel_and_provider) {
//  std::string RawJson = "{"
//                        "  \"channel\": \"my_channel_name\","
//                        "  \"channel_provider_type\": \"ca\""
//                        "}";
//
//  nlohmann::json Json = nlohmann::json::parse(RawJson);
//  BrightnESS::ForwardEpicsToKafka::MainOpt MainOpt;
//  BrightnESS::ForwardEpicsToKafka::Main Main(MainOpt);
//
//  std::string Channel;
//  std::string ProviderType;
//  Main.extractMappingInfo(Json, Channel, ProviderType);
//
//  ASSERT_EQ("my_channel_name", Channel);
//  ASSERT_EQ("ca", ProviderType);
//}
//
//class ExtractCommandsTest : public ::testing::TestWithParam<const char *> {
//  virtual void SetUp() { command = (*GetParam()); }
//  virtual void TearDown() {}
//
//protected:
//  std::string command;
//};
//
//TEST_P(ExtractCommandsTest, extracting_command_gets_command_name) {
//  std::ostringstream os;
//  os << "{"
//     << "  \"cmd\": \"" << command << "\""
//     << "}";
//
//  std::string RawJson = os.str();
//
//  nlohmann::json Json = nlohmann::json::parse(RawJson);
//  BrightnESS::ForwardEpicsToKafka::MainOpt MainOpt;
//  BrightnESS::ForwardEpicsToKafka::Main Main(MainOpt);
//  BrightnESS::ForwardEpicsToKafka::ConfigCB config(Main);
//
//  auto Cmd = config.findCommand(Json);
//
//  ASSERT_EQ(command, Cmd);
//}
//
//INSTANTIATE_TEST_CASE_P(InstantiationName, ExtractCommandsTest,
//                        ::testing::Values("add", "stop_channel", "stop_all",
//                                          "exit", "unknown_command"));
