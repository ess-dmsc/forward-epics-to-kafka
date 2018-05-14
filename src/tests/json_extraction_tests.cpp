#include "../Converter.h"
#include "../Main.h"
#include "../Config.h"
#include "ConfigParser.h"
#include "../helper.h"
#include "../CommandHandler.h"
#include <gtest/gtest.h>
#include <iostream>
#include <sstream>
#include <string>

TEST(json_extraction_tests, trying_to_parse_invalid_json_throws) {
  std::string RawJson = "{"
                        "  \"streams\": ["
                        "    }"
                        "  ]";

  BrightnESS::ForwardEpicsToKafka::ConfigParser Config;
  ASSERT_ANY_THROW(Config.setJsonFromString(RawJson));
}

TEST(json_extraction_tests, no_converters_specified_has_no_side_effects) {
  std::string RawJson = "{}";

  std::string schema("f142");

  BrightnESS::ForwardEpicsToKafka::ConfigParser Config;
  Config.setJsonFromString(RawJson);

  Config.extractGlobalConverters(schema);

  ASSERT_EQ(0u, Config.Settings.ConverterInts.size());
  ASSERT_EQ(0u, Config.Settings.ConverterStrings.size());
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

  BrightnESS::ForwardEpicsToKafka::ConfigParser Config;
  Config.setJsonFromString(RawJson);

  Config.extractGlobalConverters(schema);

  ASSERT_EQ(2u, Config.Settings.ConverterInts.size());
  ASSERT_EQ(123, Config.Settings.ConverterInts["some_option1"]);
  ASSERT_EQ(456, Config.Settings.ConverterInts["some_option2"]);
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

  BrightnESS::ForwardEpicsToKafka::ConfigParser Config;
  Config.setJsonFromString(RawJson);

  Config.extractGlobalConverters(schema);

  ASSERT_EQ(2u, Config.Settings.ConverterStrings.size());
  ASSERT_EQ("hello", Config.Settings.ConverterStrings["some_option1"]);
  ASSERT_EQ("goodbye", Config.Settings.ConverterStrings["some_option2"]);
}

TEST(json_extraction_tests,
     extracting_status_uri_gives_correct_uri_port_and_topic) {
  std::string RawJson = "{"
                        "  \"status-uri\": \"//kafkabroker:1234/status_topic\""
                        "}";

  BrightnESS::ForwardEpicsToKafka::ConfigParser Config;
  Config.setJsonFromString(RawJson);

  Config.extractConfiguration();

  ASSERT_EQ("kafkabroker", Config.Settings.StatusReportURI.host);
  ASSERT_EQ(1234u, Config.Settings.StatusReportURI.port);
  ASSERT_EQ("status_topic", Config.Settings.StatusReportURI.topic);
}

TEST(json_extraction_tests, no_status_uri_defined_gives_no_uri_port_or_topic) {
  std::string RawJson = "{}";

  BrightnESS::ForwardEpicsToKafka::ConfigParser Config;
  Config.setJsonFromString(RawJson);

  Config.extractConfiguration();

  ASSERT_EQ("", Config.Settings.StatusReportURI.host);
  ASSERT_EQ(0u, Config.Settings.StatusReportURI.port);
  ASSERT_EQ("", Config.Settings.StatusReportURI.topic);
}

TEST(json_extraction_tests, setting_broker_sets_host_and_port) {
  std::string RawJson = "{"
                        "  \"broker\": \"kafkabroker:1234\""
                        "}";

  BrightnESS::ForwardEpicsToKafka::ConfigParser Config;
  Config.setJsonFromString(RawJson);

  Config.extractConfiguration();

  ASSERT_EQ("kafkabroker", Config.Settings.Brokers[0].host);
  ASSERT_EQ(1234u, Config.Settings.Brokers[0].port);
}

TEST(json_extraction_tests,
     setting_multiple_brokers_sets_multiple_hosts_and_ports) {
  std::string RawJson = "{"
                        "  \"broker\": \"kafkabroker1:1234, kafkabroker2:5678\""
                        "}";

  BrightnESS::ForwardEpicsToKafka::ConfigParser Config;
  Config.setJsonFromString(RawJson);

  Config.extractConfiguration();

  ASSERT_EQ(2u, Config.Settings.Brokers.size());
  ASSERT_EQ("kafkabroker1", Config.Settings.Brokers[0].host);
  ASSERT_EQ(1234u, Config.Settings.Brokers[0].port);
  ASSERT_EQ("kafkabroker2", Config.Settings.Brokers[1].host);
  ASSERT_EQ(5678u, Config.Settings.Brokers[1].port);
}

TEST(json_extraction_tests, setting_no_brokers_sets_default_host_and_port) {
  std::string RawJson = "{}";

  BrightnESS::ForwardEpicsToKafka::ConfigParser Config;
  Config.setJsonFromString(RawJson);

  Config.extractConfiguration();

  ASSERT_EQ("localhost", Config.Settings.Brokers[0].host);
  ASSERT_EQ(9092u, Config.Settings.Brokers[0].port);
}

TEST(json_extraction_tests, no_kafka_broker_settings_has_no_side_effects) {
  std::string RawJson = "{}";

  BrightnESS::ForwardEpicsToKafka::ConfigParser Config;
  Config.setJsonFromString(RawJson);

  Config.extractConfiguration();

  BrightnESS::ForwardEpicsToKafka::KafkaBrokerSettings Settings = Config.Settings.BrokerSettings;

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

  BrightnESS::ForwardEpicsToKafka::ConfigParser Config;
  Config.setJsonFromString(RawJson);

  Config.extractConfiguration();

  BrightnESS::ForwardEpicsToKafka::KafkaBrokerSettings Settings = Config.Settings.BrokerSettings;

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

  BrightnESS::ForwardEpicsToKafka::ConfigParser Config;
  Config.setJsonFromString(RawJson);

  Config.extractConfiguration();

  BrightnESS::ForwardEpicsToKafka::KafkaBrokerSettings Settings = Config.Settings.BrokerSettings;

  ASSERT_EQ(2u, Settings.ConfigurationStrings.size());
  ASSERT_EQ("hello", Settings.ConfigurationStrings["some_option1"]);
  ASSERT_EQ("goodbye", Settings.ConfigurationStrings["some_option2"]);
}

TEST(json_extraction_tests,
     no_broker_config_settings_sets_default_host_port_and_topic) {
  std::string RawJson = "{}";

  BrightnESS::ForwardEpicsToKafka::ConfigParser Config;
  Config.setJsonFromString(RawJson);

  Config.extractConfiguration();

  ASSERT_EQ("localhost", Config.Settings.BrokerConfig.host);
  ASSERT_EQ(9092u, Config.Settings.BrokerConfig.port);
  ASSERT_EQ("forward_epics_to_kafka_commands", Config.Settings.BrokerConfig.topic);
}

TEST(json_extraction_tests,
     extracting_broker_config_settings_sets_host_port_and_topic) {
  std::string RawJson =
      "{"
      "  \"broker-config\": \"//kafkabroker:1234/the_command_topic\""
      "}";

  BrightnESS::ForwardEpicsToKafka::ConfigParser Config;
  Config.setJsonFromString(RawJson);

  Config.extractConfiguration();

  ASSERT_EQ("kafkabroker", Config.Settings.BrokerConfig.host);
  ASSERT_EQ(1234u, Config.Settings.BrokerConfig.port);
  ASSERT_EQ("the_command_topic", Config.Settings.BrokerConfig.topic);
}

TEST(json_extraction_tests, no_conversion_threads_settings_sets_default) {
  std::string RawJson = "{}";

  BrightnESS::ForwardEpicsToKafka::ConfigParser Config;
  Config.setJsonFromString(RawJson);

  Config.extractConfiguration();

  ASSERT_EQ(1u, Config.Settings.ConversionThreads);
}

TEST(json_extraction_tests, extracting_conversion_threads_sets_value) {
  std::string RawJson = "{"
                        "  \"conversion-threads\": 3"
                        "}";

  BrightnESS::ForwardEpicsToKafka::ConfigParser Config;
  Config.setJsonFromString(RawJson);

  Config.extractConfiguration();

  ASSERT_EQ(3u, Config.Settings.ConversionThreads);
}

TEST(json_extraction_tests, no_conversion_worker_queue_size_sets_default) {
  std::string RawJson = "{}";

  BrightnESS::ForwardEpicsToKafka::ConfigParser Config;
  Config.setJsonFromString(RawJson);

  Config.extractConfiguration();

  ASSERT_EQ(1024u, Config.Settings.ConversionWorkerQueueSize);
}

TEST(json_extraction_tests,
     extracting_conversion_worker_queue_size_sets_value) {
  std::string RawJson = "{"
                        "  \"conversion-worker-queue-size\": 1234"
                        "}";

  BrightnESS::ForwardEpicsToKafka::ConfigParser Config;
  Config.setJsonFromString(RawJson);

  Config.extractConfiguration();

  ASSERT_EQ(1234u, Config.Settings.ConversionWorkerQueueSize);
}

TEST(json_extraction_tests, no_main_poll_interval_sets_default) {
  std::string RawJson = "{}";

  BrightnESS::ForwardEpicsToKafka::ConfigParser Config;
  Config.setJsonFromString(RawJson);

  Config.extractConfiguration();

  ASSERT_EQ(500, Config.Settings.MainPollInterval);
}

TEST(json_extraction_tests, extracting_main_poll_interval_sets_value) {
  std::string RawJson = "{"
                        "  \"main-poll-interval\": 1234"
                        "}";

  BrightnESS::ForwardEpicsToKafka::ConfigParser Config;
  Config.setJsonFromString(RawJson);

  Config.extractConfiguration();

  ASSERT_EQ(1234, Config.Settings.MainPollInterval);
}

TEST(json_extraction_tests,
     extracting_streams_setting_gets_channel_and_protocol) {
  std::string RawJson = "{"
                        "  \"streams\": ["
                        "    {"
                        "      \"channel\": \"my_channel_name\","
                        "      \"channel_provider_type\": \"ca\""
                        "    }"
                        "  ]"
                        "}";

  BrightnESS::ForwardEpicsToKafka::ConfigParser Config;
  Config.setJsonFromString(RawJson);
  Config.extractConfiguration();

  ASSERT_EQ(1u, Config.Settings.StreamsInfo.size());

  auto Settings = Config.Settings.StreamsInfo[0];

  ASSERT_EQ("my_channel_name", Settings.Name);
  ASSERT_EQ("ca", Settings.EpicsProtocol);
}

TEST(json_extraction_tests,
     extracting_multiple_streams_setting_gets_channel_and_protocol) {
  std::string RawJson = "{"
                        "  \"streams\": ["
                        "    {"
                        "      \"channel\": \"my_channel_name\","
                        "      \"channel_provider_type\": \"ca\""
                        "    },"
                        "    {"
                        "      \"channel\": \"my_channel_name_2\","
                        "      \"channel_provider_type\": \"pva\""
                        "    }"
                        "  ]"
                        "}";

  BrightnESS::ForwardEpicsToKafka::ConfigParser Config;
  Config.setJsonFromString(RawJson);
  Config.extractConfiguration();

  ASSERT_EQ(2u, Config.Settings.StreamsInfo.size());

  auto Settings = Config.Settings.StreamsInfo[0];
  ASSERT_EQ("my_channel_name", Settings.Name);
  ASSERT_EQ("ca", Settings.EpicsProtocol);

  Settings = Config.Settings.StreamsInfo[1];
  ASSERT_EQ("my_channel_name_2", Settings.Name);
  ASSERT_EQ("pva", Settings.EpicsProtocol);
}

TEST(json_extraction_tests,
     extracting_streams_setting_if_protocol_not_defined_use_default) {
  std::string RawJson = "{"
                        "  \"streams\": ["
                        "    {"
                        "      \"channel\": \"my_channel_name\""
                        "    }"
                        "  ]"
                        "}";

  BrightnESS::ForwardEpicsToKafka::ConfigParser Config;
  Config.setJsonFromString(RawJson);
  Config.extractConfiguration();

  ASSERT_EQ(1u, Config.Settings.StreamsInfo.size());

  auto Settings = Config.Settings.StreamsInfo[0];

  ASSERT_EQ("my_channel_name", Settings.Name);
  ASSERT_EQ("pva", Settings.EpicsProtocol);
}

TEST(json_extraction_tests,
     extracting_streams_setting_gets_converter_info) {

  std::string RawJson = "{"
                        "  \"streams\": ["
                        "    {"
                        "      \"channel\": \"my_channel_name\","
                        "      \"channel_provider_type\": \"ca\","
                        "      \"converter\": {"
                        "        \"schema\": \"f142\", "
                        "        \"topic\": \"Kafka_topic_name\", "
                        "        \"name\": \"my_name\""
                        "      }"
                        "    }"
                        "  ]"
                        "}";

  BrightnESS::ForwardEpicsToKafka::ConfigParser Config;
  Config.setJsonFromString(RawJson);
  Config.extractConfiguration();

  auto Settings = Config.Settings.StreamsInfo[0].Converters[0];

  ASSERT_EQ("f142", Settings.Schema);
  ASSERT_EQ("Kafka_topic_name", Settings.Topic);
  ASSERT_EQ("my_name", Settings.Name);
}

TEST(json_extraction_tests,
     extracting_converter_info_with_no_name_gets_auto_named) {
  std::string RawJson = "{"
                        "  \"streams\": ["
                        "    {"
                        "      \"channel\": \"my_channel_name\","
                        "      \"channel_provider_type\": \"ca\","
                        "      \"converter\": {"
                        "        \"schema\": \"f142\", "
                        "        \"topic\": \"Kafka_topic_name\""
                        "      }"
                        "    }"
                        "  ]"
                        "}";

  BrightnESS::ForwardEpicsToKafka::ConfigParser Config;
  Config.setJsonFromString(RawJson);
  Config.extractConfiguration();

  auto Settings = Config.Settings.StreamsInfo[0].Converters[0];

  // Don't care what the name is, but it must be something
  ASSERT_TRUE(!Settings.Name.empty());
}

TEST(json_extraction_tests, extracting_converter_info_with_no_topic_throws) {
  std::string RawJson = "{"
                        "  \"streams\": ["
                        "    {"
                        "      \"channel\": \"my_channel_name\","
                        "      \"channel_provider_type\": \"ca\","
                        "      \"converter\": {"
                        "        \"schema\": \"f142\""
                        "      }"
                        "    }"
                        "  ]"
                        "}";

  BrightnESS::ForwardEpicsToKafka::ConfigParser Config;
  Config.setJsonFromString(RawJson);

  ASSERT_ANY_THROW(Config.extractConfiguration());
}

TEST(json_extraction_tests, extracting_converter_info_with_no_schema_throws) {
  std::string RawJson = "{"
                        "  \"streams\": ["
                        "    {"
                        "      \"channel\": \"my_channel_name\","
                        "      \"channel_provider_type\": \"ca\","
                        "      \"converter\": {"
                        "        \"topic\": \"Kafka_topic_name\""
                        "      }"
                        "    }"
                        "  ]"
                        "}";

  BrightnESS::ForwardEpicsToKafka::ConfigParser Config;
  Config.setJsonFromString(RawJson);

  ASSERT_ANY_THROW(Config.extractConfiguration());
}

class ExtractCommandsTest : public ::testing::TestWithParam<const char *> {
  virtual void SetUp() { command = (*GetParam()); }
  virtual void TearDown() {}

protected:
  std::string command;
};

TEST_P(ExtractCommandsTest, extracting_command_gets_command_name) {
  std::ostringstream os;
  os << "{"
     << "  \"cmd\": \"" << command << "\""
     << "}";

  std::string RawJson = os.str();

  nlohmann::json Json = nlohmann::json::parse(RawJson);
  BrightnESS::ForwardEpicsToKafka::MainOpt MainOpt;
  BrightnESS::ForwardEpicsToKafka::Main Main(MainOpt);
  BrightnESS::ForwardEpicsToKafka::ConfigCB config(Main);

  auto Cmd = config.findCommand(Json);

  ASSERT_EQ(command, Cmd);
}

INSTANTIATE_TEST_CASE_P(InstantiationName, ExtractCommandsTest,
                        ::testing::Values("add", "stop_channel", "stop_all",
                                          "exit", "unknown_command"));
