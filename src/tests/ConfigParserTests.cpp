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
  std::string Brokers = "//localhost1:9092, //localhost2:9092";
  Forwarder::ConfigParser::setBrokers(Brokers, MainOpt.MainSettings);
  ASSERT_EQ("localhost1:9092", MainOpt.MainSettings.Brokers.at(0).HostPort);
  ASSERT_EQ("localhost2:9092", MainOpt.MainSettings.Brokers.at(1).HostPort);
}

TEST(ConfigParserTest,
     set_brokers_with_one_broker_and_topic_parses_one_broker) {
  Forwarder::MainOpt MainOpt;
  std::string Brokers = "//localhost:9092/TEST_forwarderConfig";
  Forwarder::ConfigParser::setBrokers(Brokers, MainOpt.MainSettings);
  ASSERT_EQ("localhost:9092", MainOpt.MainSettings.Brokers.at(0).HostPort);
  ASSERT_EQ(9092, MainOpt.MainSettings.Brokers.at(0).Port);
  ASSERT_EQ("TEST_forwarderConfig", MainOpt.MainSettings.Brokers.at(0).Topic);
  ASSERT_EQ(1, MainOpt.MainSettings.Brokers.size());
}

TEST(ConfigParserTest, trying_to_parse_invalid_json_throws) {
  std::string RawJson = R"({
                            "streams": [
                             }
                           ])";

  ASSERT_ANY_THROW(Forwarder::ConfigParser Config(RawJson));
}

TEST(ConfigParserTest, no_streams_object_throws) {
  std::string RawJson = R"({
                            "streams": [1]
                           })";

  Forwarder::ConfigParser Config(RawJson);

  ASSERT_THROW(Forwarder::ConfigSettings Settings = Config.extractStreamInfo(),
               Forwarder::MappingAddException);
}

TEST(ConfigParserTest, no_channel_found_throws) {
  std::string RawJson = R"({
                            "streams": [{"a": "b"}]
                           })";

  Forwarder::ConfigParser Config(RawJson);

  ASSERT_THROW(Forwarder::ConfigSettings Settings = Config.extractStreamInfo(),
               Forwarder::MappingAddException);
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

TEST(
    ConfigParserTest,
    extracting_converter_info_with_schema_array_appends_to_converter_settings) {
  std::string RawJson = R"({
                            "streams": [
                               {
                                 "channel": "my_channel_name",
                                 "channel_provider_type": "ca",
                                 "converter": [
                                    {
                                      "topic": "Kafka_topic_name",
                                      "schema" : "f142"
                                    },
                                    {
                                      "topic": "Another_topic",
                                      "schema" : "f142"
                                    }
                                 ]
                               }
                            ]
                           })";

  Forwarder::ConfigParser Config(RawJson);
  Forwarder::ConfigSettings Settings = Config.extractStreamInfo();

  auto Converter1 = Settings.StreamsInfo.at(0).Converters.at(0);
  auto Converter2 = Settings.StreamsInfo.at(0).Converters.at(1);

  ASSERT_EQ("Kafka_topic_name", Converter1.Topic);
  ASSERT_EQ("Another_topic", Converter2.Topic);
}

TEST(ConfigParserTest, setBrokers_with_comma) {
  Forwarder::ConfigSettings Settings;
  Forwarder::ConfigParser::setBrokers("//hello, //world!", Settings);
  Forwarder::URI first("//hello");
  Forwarder::URI second("//world!");
  ASSERT_EQ(first.HostPort, Settings.Brokers.at(0).HostPort);
  ASSERT_EQ(second.HostPort, Settings.Brokers.at(1).HostPort);
}

TEST(ConfigParserTest, setBrokers_single_item) {
  Forwarder::ConfigSettings Settings;
  Forwarder::ConfigParser::setBrokers("//abc", Settings);
  Forwarder::URI first("//abc");
  ASSERT_EQ(first.HostPort, Settings.Brokers.at(0).HostPort);
  ASSERT_EQ(1, Settings.Brokers.size());
}

TEST(ConfigParserTest, setBrokers_with_comma_before_brokers) {
  Forwarder::ConfigSettings Settings;
  Forwarder::ConfigParser::setBrokers(",//a,//b", Settings);
  Forwarder::URI first("//a");
  Forwarder::URI second("//b");
  ASSERT_EQ(first.HostPort, Settings.Brokers.at(0).HostPort);
  ASSERT_EQ(second.HostPort, Settings.Brokers.at(1).HostPort);
}

TEST(
    ConfigParserTest,
    setBrokers_does_not_split_all_characters_and_returns_vector_of_words_between_split_character) {
  Forwarder::ConfigSettings Settings;
  Forwarder::ConfigParser::setBrokers("//ac,//dc,", Settings);
  Forwarder::URI first("//ac");
  Forwarder::URI second("//dc");
  ASSERT_EQ(first.HostPort, Settings.Brokers.at(0).HostPort);
  ASSERT_EQ(second.HostPort, Settings.Brokers.at(1).HostPort);
}

TEST(
    ConfigParserTest,
    setBrokers_adds_no_blank_characters_with_character_before_and_after_string) {
  Forwarder::ConfigSettings Settings;
  Forwarder::ConfigParser::setBrokers(",//ac,//dc,", Settings);
  Forwarder::URI first("//ac");
  Forwarder::URI second("//dc");
  ASSERT_EQ(first.HostPort, Settings.Brokers.at(0).HostPort);
  ASSERT_EQ(second.HostPort, Settings.Brokers.at(1).HostPort);
}

TEST(ConfigParserTest,
     setBrokers_adds_multiple_words_before_and_after_characters) {
  Forwarder::ConfigSettings Settings;
  Forwarder::ConfigParser::setBrokers(
      ",//some,//longer,//thing,//for,//testing", Settings);
  Forwarder::URI first("//some");
  Forwarder::URI second("//longer");
  Forwarder::URI third("//thing");
  Forwarder::URI fourth("//for");
  Forwarder::URI fifth("//testing");
  ASSERT_EQ(5, Settings.Brokers.size());
  ASSERT_EQ(first.HostPort, Settings.Brokers.at(0).HostPort);
  ASSERT_EQ(second.HostPort, Settings.Brokers.at(1).HostPort);
  ASSERT_EQ(third.HostPort, Settings.Brokers.at(2).HostPort);
  ASSERT_EQ(fourth.HostPort, Settings.Brokers.at(3).HostPort);
  ASSERT_EQ(fifth.HostPort, Settings.Brokers.at(4).HostPort);
}
