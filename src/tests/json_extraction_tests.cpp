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
}

TEST(json_extraction_tests, extracting_status_uri_gives_correct_uri_and_port) {
  std::string StatusJson = "{"
                           "  \"status-uri\": \"//kafkabroker:1234/the_status_topic\""
                           "}";

  nlohmann::json Json = nlohmann::json::parse(StatusJson);
  BrightnESS::ForwardEpicsToKafka::MainOpt main;
  main.JSONConfiguration = Json;
  main.find_status_uri();

  ASSERT_EQ("kafkabroker", main.StatusReportURI.host);
  ASSERT_EQ(1234u, main.StatusReportURI.port);
}

TEST(json_extraction_tests, no_status_uri_defined_gives_no_uri_and_port) {
  std::string StatusJson = "{"
                           "}";

  nlohmann::json Json = nlohmann::json::parse(StatusJson);
  BrightnESS::ForwardEpicsToKafka::MainOpt main;
  main.JSONConfiguration = Json;
  main.find_status_uri();

  ASSERT_EQ("", main.StatusReportURI.host);
  ASSERT_EQ(0u, main.StatusReportURI.port);
}