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

TEST(ConfigParserTest, using_broker_list_creates_multiple_brokers_) {
  Forwarder::MainOpt MainOpt;
  Forwarder::ConfigParser Config("{}");
  std::string Brokers = "localhost1,localhost2";
  Config.setBrokers(Brokers, MainOpt.MainSettings);
  ASSERT_EQ("localhost1", MainOpt.MainSettings.Brokers.at(0).host);
  ASSERT_EQ("localhost2", MainOpt.MainSettings.Brokers.at(1).host);
  ASSERT_EQ(Brokers, MainOpt.brokers_as_comma_list());
}

TEST(ConfigParserTest,
     set_brokers_with_one_broker_and_topic_parses_one_broker) {
  Forwarder::MainOpt MainOpt;
  Forwarder::ConfigParser Config("{}");
  std::string Brokers = "localhost:9092/TEST_forwarderConfig";
  Config.setBrokers(Brokers, MainOpt.MainSettings);
  ASSERT_EQ("localhost", MainOpt.MainSettings.Brokers.at(0).host);
  ASSERT_EQ(9092, MainOpt.MainSettings.Brokers.at(0).port);
  ASSERT_EQ("TEST_forwarderConfig", MainOpt.MainSettings.Brokers.at(0).topic);
  ASSERT_EQ(1, MainOpt.MainSettings.Brokers.size());
  ASSERT_EQ("localhost:9092", MainOpt.brokers_as_comma_list());
}

TEST(ConfigParserTest, trying_to_parse_invalid_json_throws) {
  std::string RawJson = R"({
                            "streams": [
                             }
                           ])";

  ASSERT_ANY_THROW(Forwarder::ConfigParser Config(RawJson));
}

TEST(ConfigParserTest, no_converters_specified_has_no_side_effects) {
  std::string RawJson = "{}";

  Forwarder::ConfigParser Config(RawJson);

  Forwarder::ConfigSettings Settings = Config.extractStreamInfo();

  ASSERT_EQ(0u, Settings.GlobalConverters.size());
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

  Forwarder::ConfigParser Config(RawJson);
  Forwarder::ConfigSettings Settings = Config.extractStreamInfo();

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

  Forwarder::ConfigParser Config(RawJson);
  Forwarder::ConfigSettings Settings = Config.extractStreamInfo();

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

  Forwarder::ConfigParser Config(RawJson);
  Forwarder::ConfigSettings Settings = Config.extractStreamInfo();

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

  Forwarder::ConfigParser Config(RawJson);
  Forwarder::ConfigSettings Settings = Config.extractStreamInfo();

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

  Forwarder::ConfigParser Config(RawJson);
  Forwarder::ConfigSettings Settings = Config.extractStreamInfo();

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

  Forwarder::ConfigParser Config(RawJson);

  ASSERT_ANY_THROW(Config.extractStreamInfo());
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

  Forwarder::ConfigParser Config(RawJson);

  ASSERT_ANY_THROW(Config.extractStreamInfo());
}
