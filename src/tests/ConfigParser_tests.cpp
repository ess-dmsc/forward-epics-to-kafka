#include "../CommandHandler.h"
#include "../Config.h"
#include "../Converter.h"
#include "../MainOpt.h"
#include "../helper.h"
#include "ConfigParser.h"
#include "Forwarder.h"
#include <gtest/gtest.h>
#include <iostream>
#include <sstream>
#include <string>

TEST(ConfigParserTest, not_parsing_a_config_file_gives_defaults) {
  Forwarder::MainOpt MainOpt;

  ASSERT_EQ(1u, MainOpt.MainSettings.ConversionThreads);
}

TEST(ConfigParserTest, trying_to_parse_invalid_json_throws) {
  std::string RawJson = R"({
                            "streams": [
                             }
                           ])";

  Forwarder::ConfigParser Config;
  ASSERT_ANY_THROW(Config.setJsonFromString(RawJson));
}

TEST(ConfigParserTest, no_converters_specified_has_no_side_effects) {
  std::string RawJson = "{}";

  Forwarder::ConfigParser Config;
  Config.setJsonFromString(RawJson);

  Forwarder::ConfigSettings Settings = Config.extractConfiguration();

  ASSERT_EQ(0u, Settings.GlobalConverters.size());
}

TEST(ConfigParserTest, ints_specified_in_converters_are_extracted) {
  std::string RawJson = R"({
                            "converters": {
                               "f142": {
                                 "some_option1": 123,
                                 "some_option2": 456
                               }
                            }
                           })";

  Forwarder::ConfigParser Config;
  Config.setJsonFromString(RawJson);

  Forwarder::ConfigSettings Settings = Config.extractConfiguration();

  ASSERT_EQ(1u, Settings.GlobalConverters.size());

  auto &f142 = Settings.GlobalConverters.at("f142");
  ASSERT_EQ(2u, f142.ConfigurationIntegers.size());
  ASSERT_EQ(123, f142.ConfigurationIntegers.at("some_option1"));
  ASSERT_EQ(456, f142.ConfigurationIntegers.at("some_option2"));
}

TEST(ConfigParserTest, strings_specified_in_converters_are_extracted) {
  std::string RawJson = R"({
                            "converters": {
                               "f142": {
                                 "some_option1": "hello",
                                 "some_option2": "goodbye"
                               }
                            }
                           })";

  Forwarder::ConfigParser Config;
  Config.setJsonFromString(RawJson);

  Forwarder::ConfigSettings Settings = Config.extractConfiguration();

  ASSERT_EQ(1u, Settings.GlobalConverters.size());

  auto &f142 = Settings.GlobalConverters.at("f142");
  ASSERT_EQ(2u, f142.ConfigurationStrings.size());
  ASSERT_EQ("hello", f142.ConfigurationStrings.at("some_option1"));
  ASSERT_EQ("goodbye", f142.ConfigurationStrings.at("some_option2"));
}

TEST(ConfigParserTest, extracting_streams_setting_gets_channel_and_protocol) {
  std::string RawJson = R"({
                            "streams": [
                               {
                                 "channel": "my_channel_name",
                                 "channel_provider_type": "ca"
                               }
                            ]
                           })";

  Forwarder::ConfigParser Config;
  Config.setJsonFromString(RawJson);
  Forwarder::ConfigSettings Settings = Config.extractConfiguration();

  ASSERT_EQ(1u, Settings.StreamsInfo.size());

  auto Converter = Settings.StreamsInfo.at(0);

  ASSERT_EQ("my_channel_name", Converter.Name);
  ASSERT_EQ("ca", Converter.EpicsProtocol);
}

TEST(ConfigParserTest,
     extracting_multiple_streams_setting_gets_channel_and_protocol) {
  std::string RawJson = R"({
                            "streams": [
                               {
                                 "channel": "my_channel_name",
                                 "channel_provider_type": "ca"
                               },
                               {
                                 "channel": "my_channel_name_2",
                                 "channel_provider_type": "pva"
                               }
                            ]
                           })";

  Forwarder::ConfigParser Config;
  Config.setJsonFromString(RawJson);
  Forwarder::ConfigSettings Settings = Config.extractConfiguration();

  ASSERT_EQ(2u, Settings.StreamsInfo.size());

  auto Converter = Settings.StreamsInfo.at(0);
  ASSERT_EQ("my_channel_name", Converter.Name);
  ASSERT_EQ("ca", Converter.EpicsProtocol);

  Converter = Settings.StreamsInfo.at(1);
  ASSERT_EQ("my_channel_name_2", Converter.Name);
  ASSERT_EQ("pva", Converter.EpicsProtocol);
}

TEST(ConfigParserTest,
     extracting_streams_setting_if_protocol_not_defined_use_default) {
  std::string RawJson = R"({
                            "streams": [
                               {
                                 "channel": "my_channel_name"
                               }
                            ]
                           })";

  Forwarder::ConfigParser Config;
  Config.setJsonFromString(RawJson);
  Forwarder::ConfigSettings Settings = Config.extractConfiguration();

  ASSERT_EQ(1u, Settings.StreamsInfo.size());

  auto Converter = Settings.StreamsInfo.at(0);

  ASSERT_EQ("my_channel_name", Converter.Name);
  ASSERT_EQ("pva", Converter.EpicsProtocol);
}

TEST(ConfigParserTest, extracting_streams_setting_gets_converter_info) {

  std::string RawJson = R"({
                            "streams": [
                               {
                                 "channel": "my_channel_name",
                                 "channel_provider_type": "ca",
                                 "converter": {
                                   "schema": "f142",
                                   "topic": "Kafka_topic_name",
                                   "name": "my_name"
                                 }
                               }
                            ]
                           })";

  Forwarder::ConfigParser Config;
  Config.setJsonFromString(RawJson);
  Forwarder::ConfigSettings Settings = Config.extractConfiguration();

  auto Converter = Settings.StreamsInfo.at(0).Converters.at(0);

  ASSERT_EQ("f142", Converter.Schema);
  ASSERT_EQ("Kafka_topic_name", Converter.Topic);
  ASSERT_EQ("my_name", Converter.Name);
}

TEST(ConfigParserTest, extracting_converter_info_with_no_name_gets_auto_named) {
  std::string RawJson = R"({
                            "streams": [
                               {
                                 "channel": "my_channel_name",
                                 "channel_provider_type": "ca",
                                 "converter": {
                                    "schema": "f142",
                                    "topic": "Kafka_topic_name"
                                 }
                               }
                            ]
                           })";

  Forwarder::ConfigParser Config;
  Config.setJsonFromString(RawJson);
  Forwarder::ConfigSettings Settings = Config.extractConfiguration();

  auto Converter = Settings.StreamsInfo.at(0).Converters.at(0);

  // Don't care what the name is, but it must be something
  ASSERT_TRUE(!Converter.Name.empty());
}

TEST(ConfigParserTest, extracting_converter_info_with_no_topic_throws) {
  std::string RawJson = R"({
                            "streams": [
                               {
                                 "channel": "my_channel_name",
                                 "channel_provider_type": "ca",
                                 "converter": {
                                    "schema": "f142"
                                 }
                               }
                            ]
                           })";

  Forwarder::ConfigParser Config;
  Config.setJsonFromString(RawJson);

  ASSERT_ANY_THROW(Config.extractConfiguration());
}

TEST(ConfigParserTest, extracting_converter_info_with_no_schema_throws) {
  std::string RawJson = R"({
                            "streams": [
                               {
                                 "channel": "my_channel_name",
                                 "channel_provider_type": "ca",
                                 "converter": {
                                    "topic": "Kafka_topic_name"
                                 }
                               }
                            ]
                           })";

  Forwarder::ConfigParser Config;
  Config.setJsonFromString(RawJson);

  ASSERT_ANY_THROW(Config.extractConfiguration());
}
