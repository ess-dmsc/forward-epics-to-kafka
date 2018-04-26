#include <string>
#include <iostream>
#include <gtest/gtest.h>
#include "../Converter.h"
#include "../MainOpt.h"

TEST(json_extraction_tests, no_converters_specified_has_no_side_effects) {
  std::string NoConvertersJson = "{"
                                 "}";

  nlohmann::json Json = nlohmann::json::parse(NoConvertersJson);
  std::map<std::string, int64_t> config_ints;
  std::map<std::string, std::string> config_strings;
  std::string schema("f142");

  BrightnESS::ForwardEpicsToKafka::Converter::extractConfig(schema, Json, config_ints, config_strings);

  ASSERT_EQ(0u, config_ints.size());
  ASSERT_EQ(0u, config_strings.size());
}

TEST(json_extraction_tests, ints_specified_in_converters_is_extracted) {
  std::string ConvertersJson = "{"
                               "  \"converters\": {"
                               "    \"f142\": { "
                               "      \"some_option1\": 123, "
                               "      \"some_option2\": 456"
                               "    }"
                               "  }"
                               "}";

  nlohmann::json Json = nlohmann::json::parse(ConvertersJson);
  std::map<std::string, int64_t> config_ints;
  std::map<std::string, std::string> config_strings;
  std::string schema("f142");

  BrightnESS::ForwardEpicsToKafka::Converter::extractConfig(schema, Json, config_ints, config_strings);

  ASSERT_EQ(2u, config_ints.size());
  ASSERT_EQ(123, config_ints["some_option1"]);
  ASSERT_EQ(456, config_ints["some_option2"]);
}

TEST(json_extraction_tests, strings_specified_in_converters_is_extracted) {
  std::string ConvertersJson = "{"
                               "  \"converters\": {"
                               "    \"f142\": { "
                               "      \"some_option1\": \"hello\", "
                               "      \"some_option2\": \"goodbye\""
                               "    }"
                               "  }"
                               "}";

  nlohmann::json Json = nlohmann::json::parse(ConvertersJson);
  std::map<std::string, int64_t> config_ints;
  std::map<std::string, std::string> config_strings;
  std::string schema("f142");

  BrightnESS::ForwardEpicsToKafka::Converter::extractConfig(schema, Json, config_ints, config_strings);

  ASSERT_EQ(2u, config_strings.size());
  ASSERT_EQ("hello", config_strings["some_option1"]);
  ASSERT_EQ("goodbye", config_strings["some_option2"]);
}

TEST(json_extraction_tests, extracting_status_uri_gives_correct_uri_port_and_topic) {
  std::string StatusJson = "{"
                           "  \"status-uri\": \"//kafkabroker:1234/the_status_topic\""
                           "}";

  nlohmann::json Json = nlohmann::json::parse(StatusJson);
  BrightnESS::ForwardEpicsToKafka::MainOpt main;
  main.JSONConfiguration = Json;
  main.find_status_uri();

  ASSERT_EQ("kafkabroker", main.StatusReportURI.host);
  ASSERT_EQ(1234u, main.StatusReportURI.port);
  ASSERT_EQ("the_status_topic", main.StatusReportURI.topic);
}

TEST(json_extraction_tests, no_status_uri_defined_gives_no_uri_port_or_topic) {
  std::string StatusJson = "{"
                           "}";

  nlohmann::json Json = nlohmann::json::parse(StatusJson);
  BrightnESS::ForwardEpicsToKafka::MainOpt main;
  main.JSONConfiguration = Json;
  main.find_status_uri();

  ASSERT_EQ("", main.StatusReportURI.host);
  ASSERT_EQ(0u, main.StatusReportURI.port);
  ASSERT_EQ("", main.StatusReportURI.topic);
}

TEST(json_extraction_tests, setting_broker_sets_host_and_port) {
  std::string StatusJson = "{"
                           "  \"broker\": \"kafkabroker:1234\""
                           "}";

  nlohmann::json Json = nlohmann::json::parse(StatusJson);
  BrightnESS::ForwardEpicsToKafka::MainOpt main;
  main.JSONConfiguration = Json;
  main.find_broker();

  ASSERT_EQ("kafkabroker", main.brokers[0].host);
  ASSERT_EQ(1234u, main.brokers[0].port);
}

TEST(json_extraction_tests, setting_multiple_brokers_sets_multiple_hosts_and_ports) {
  std::string StatusJson = "{"
                           "  \"broker\": \"kafkabroker1:1234, kafkabroker2:5678\""
                           "}";

  nlohmann::json Json = nlohmann::json::parse(StatusJson);
  BrightnESS::ForwardEpicsToKafka::MainOpt main;
  main.JSONConfiguration = Json;
  main.find_broker();

  ASSERT_EQ(2u, main.brokers.size());
  ASSERT_EQ("kafkabroker1", main.brokers[0].host);
  ASSERT_EQ(1234u, main.brokers[0].port);
  ASSERT_EQ("kafkabroker2", main.brokers[1].host);
  ASSERT_EQ(5678u, main.brokers[1].port);
}

TEST(json_extraction_tests, setting_no_brokers_sets_default_host_and_port) {
  std::string StatusJson = "{"
                           "}";

  nlohmann::json Json = nlohmann::json::parse(StatusJson);
  BrightnESS::ForwardEpicsToKafka::MainOpt main;
  main.JSONConfiguration = Json;
  main.find_broker();

  ASSERT_EQ("localhost", main.brokers[0].host);
  ASSERT_EQ(9092u, main.brokers[0].port);
}

TEST(json_extraction_tests, no_kafka_broker_settings_has_no_side_effects) {
  std::string NoBrokerJson = "{"
                           "}";

  nlohmann::json Json = nlohmann::json::parse(NoBrokerJson);

  auto Settings = BrightnESS::ForwardEpicsToKafka::extractKafkaBrokerSettingsFromJSON(Json);

  ASSERT_EQ(0u, Settings.ConfigurationIntegers.size());
  ASSERT_EQ(0u, Settings.ConfigurationStrings.size());
}

TEST(json_extraction_tests, ints_in_kafka_broker_settings_are_extracted) {
  std::string NoBrokerJson = "{"
                             "  \"kafka\": {"
                             "    \"broker\": { "
                             "      \"some_option1\": 123, "
                             "      \"some_option2\": 456"
                             "    }"
                             "  }"
                             "}";

  nlohmann::json Json = nlohmann::json::parse(NoBrokerJson);

  auto Settings = BrightnESS::ForwardEpicsToKafka::extractKafkaBrokerSettingsFromJSON(Json);

  ASSERT_EQ(2u, Settings.ConfigurationIntegers.size());
  ASSERT_EQ(123, Settings.ConfigurationIntegers["some_option1"]);
  ASSERT_EQ(456, Settings.ConfigurationIntegers["some_option2"]);
}

TEST(json_extraction_tests, strings_in_kafka_broker_settings_are_extracted) {
  std::string NoBrokerJson = "{"
                             "  \"kafka\": {"
                             "    \"broker\": { "
                             "      \"some_option1\": \"hello\", "
                             "      \"some_option2\": \"goodbye\""
                             "    }"
                             "  }"
                             "}";

  nlohmann::json Json = nlohmann::json::parse(NoBrokerJson);

  auto Settings = BrightnESS::ForwardEpicsToKafka::extractKafkaBrokerSettingsFromJSON(Json);

  ASSERT_EQ(2u, Settings.ConfigurationStrings.size());
  ASSERT_EQ("hello", Settings.ConfigurationStrings["some_option1"]);
  ASSERT_EQ("goodbye", Settings.ConfigurationStrings["some_option2"]);
}
