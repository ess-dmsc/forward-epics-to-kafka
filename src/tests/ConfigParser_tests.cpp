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

TEST(ConfigParserTest, values_specified_in_multiple_converters_are_extracted) {
  std::string RawJson = R"({
                            "converters": {
                               "f142": {
                                 "some_option1": 123,
                                 "some_option2": "goodbye"
                               },
                               "f143": {
                                 "some_option3": "hello",
                                 "some_option4": 456
                               }
                            }
                           })";

  Forwarder::ConfigParser Config;
  Config.setJsonFromString(RawJson);

  Forwarder::ConfigSettings Settings = Config.extractConfiguration();

  ASSERT_EQ(2u, Settings.GlobalConverters.size());

  auto &f142 = Settings.GlobalConverters.at("f142");
  ASSERT_EQ(1u, f142.ConfigurationIntegers.size());
  ASSERT_EQ(1u, f142.ConfigurationStrings.size());
  ASSERT_EQ(123, f142.ConfigurationIntegers.at("some_option1"));
  ASSERT_EQ("goodbye", f142.ConfigurationStrings.at("some_option2"));

  auto &f143 = Settings.GlobalConverters.at("f143");
  ASSERT_EQ(1u, f143.ConfigurationIntegers.size());
  ASSERT_EQ(1u, f142.ConfigurationStrings.size());
  ASSERT_EQ("hello", f143.ConfigurationStrings.at("some_option3"));
  ASSERT_EQ(456, f143.ConfigurationIntegers.at("some_option4"));
}

TEST(ConfigParserTest, extracting_status_uri_gives_correct_uri_port_and_topic) {
  std::string RawJson = R"({
                            "status-uri": "//kafkabroker:1234/status_topic"
                           })";

  Forwarder::ConfigParser Config;
  Config.setJsonFromString(RawJson);

  Forwarder::ConfigSettings Settings = Config.extractConfiguration();

  ASSERT_EQ("kafkabroker", Settings.StatusReportURI.host);
  ASSERT_EQ(1234u, Settings.StatusReportURI.port);
  ASSERT_EQ("status_topic", Settings.StatusReportURI.topic);
}

TEST(ConfigParserTest, no_status_uri_defined_gives_no_uri_port_or_topic) {
  std::string RawJson = "{}";

  Forwarder::ConfigParser Config;
  Config.setJsonFromString(RawJson);

  Forwarder::ConfigSettings Settings = Config.extractConfiguration();

  ASSERT_EQ("", Settings.StatusReportURI.host);
  ASSERT_EQ(0u, Settings.StatusReportURI.port);
  ASSERT_EQ("", Settings.StatusReportURI.topic);
}

TEST(ConfigParserTest, setting_broker_sets_host_and_port) {
  std::string RawJson = R"({
                            "broker": "kafkabroker:1234"
                           })";

  Forwarder::ConfigParser Config;
  Config.setJsonFromString(RawJson);

  Forwarder::ConfigSettings Settings = Config.extractConfiguration();

  ASSERT_EQ("kafkabroker", Settings.Brokers.at(0).host);
  ASSERT_EQ(1234u, Settings.Brokers.at(0).port);
}

TEST(ConfigParserTest, setting_multiple_brokers_sets_multiple_hosts_and_ports) {
  std::string RawJson = R"({
                            "broker": "kafkabroker1:1234, kafkabroker2:5678"
                           })";

  Forwarder::ConfigParser Config;
  Config.setJsonFromString(RawJson);

  Forwarder::ConfigSettings Settings = Config.extractConfiguration();

  ASSERT_EQ(2u, Settings.Brokers.size());
  ASSERT_EQ("kafkabroker1", Settings.Brokers.at(0).host);
  ASSERT_EQ(1234u, Settings.Brokers.at(0).port);
  ASSERT_EQ("kafkabroker2", Settings.Brokers.at(1).host);
  ASSERT_EQ(5678u, Settings.Brokers.at(1).port);
}

TEST(ConfigParserTest, setting_no_brokers_sets_default_host_and_port) {
  std::string RawJson = "{}";

  Forwarder::ConfigParser Config;
  Config.setJsonFromString(RawJson);

  Forwarder::ConfigSettings Settings = Config.extractConfiguration();

  ASSERT_EQ("localhost", Settings.Brokers.at(0).host);
  ASSERT_EQ(9092u, Settings.Brokers.at(0).port);
}

TEST(ConfigParserTest, no_kafka_broker_settings_has_no_side_effects) {
  std::string RawJson = "{}";

  Forwarder::ConfigParser Config;
  Config.setJsonFromString(RawJson);

  Forwarder::ConfigSettings Settings = Config.extractConfiguration();

  ASSERT_EQ(0u, Settings.BrokerSettings.ConfigurationIntegers.size());
  ASSERT_EQ(0u, Settings.BrokerSettings.ConfigurationStrings.size());
}

TEST(ConfigParserTest, ints_in_kafka_broker_settings_are_extracted) {
  std::string RawJson = R"({
                            "kafka": {
                               "broker": {
                                 "some_option1": 123,
                                 "some_option2": 456
                               }
                            }
                           })";

  Forwarder::ConfigParser Config;
  Config.setJsonFromString(RawJson);

  Forwarder::ConfigSettings Settings = Config.extractConfiguration();

  ASSERT_EQ(2u, Settings.BrokerSettings.ConfigurationIntegers.size());
  ASSERT_EQ(123,
            Settings.BrokerSettings.ConfigurationIntegers.at("some_option1"));
  ASSERT_EQ(456,
            Settings.BrokerSettings.ConfigurationIntegers.at("some_option2"));
}

TEST(ConfigParserTest, strings_in_kafka_broker_settings_are_extracted) {
  std::string RawJson = R"({
                            "kafka": {
                               "broker": {
                                 "some_option1": "hello",
                                 "some_option2": "goodbye"
                               }
                            }
                           })";

  Forwarder::ConfigParser Config;
  Config.setJsonFromString(RawJson);

  Forwarder::ConfigSettings Settings = Config.extractConfiguration();

  ASSERT_EQ(2u, Settings.BrokerSettings.ConfigurationStrings.size());
  ASSERT_EQ("hello",
            Settings.BrokerSettings.ConfigurationStrings.at("some_option1"));
  ASSERT_EQ("goodbye",
            Settings.BrokerSettings.ConfigurationStrings.at("some_option2"));
}

TEST(ConfigParserTest,
     no_broker_config_settings_sets_default_host_port_and_topic) {
  std::string RawJson = "{}";

  Forwarder::ConfigParser Config;
  Config.setJsonFromString(RawJson);

  Forwarder::ConfigSettings Settings = Config.extractConfiguration();

  ASSERT_EQ("localhost", Settings.BrokerConfig.host);
  ASSERT_EQ(9092u, Settings.BrokerConfig.port);
  ASSERT_EQ("forward_epics_to_kafka_commands", Settings.BrokerConfig.topic);
}

TEST(ConfigParserTest,
     extracting_broker_config_settings_sets_host_port_and_topic) {
  std::string RawJson = R"({
                            "broker-config": "//kafkabroker:1234/the_topic"
                           })";

  Forwarder::ConfigParser Config;
  Config.setJsonFromString(RawJson);

  Forwarder::ConfigSettings Settings = Config.extractConfiguration();

  ASSERT_EQ("kafkabroker", Settings.BrokerConfig.host);
  ASSERT_EQ(1234u, Settings.BrokerConfig.port);
  ASSERT_EQ("the_topic", Settings.BrokerConfig.topic);
}

TEST(ConfigParserTest, no_conversion_threads_settings_sets_default) {
  std::string RawJson = "{}";

  Forwarder::ConfigParser Config;
  Config.setJsonFromString(RawJson);

  Forwarder::ConfigSettings Settings = Config.extractConfiguration();

  ASSERT_EQ(1u, Settings.ConversionThreads);
}

TEST(ConfigParserTest, extracting_conversion_threads_sets_value) {
  std::string RawJson = R"({
                            "conversion-threads": 3
                           })";

  Forwarder::ConfigParser Config;
  Config.setJsonFromString(RawJson);

  Forwarder::ConfigSettings Settings = Config.extractConfiguration();

  ASSERT_EQ(3u, Settings.ConversionThreads);
}

TEST(ConfigParserTest, no_conversion_worker_queue_size_sets_default) {
  std::string RawJson = "{}";

  Forwarder::ConfigParser Config;
  Config.setJsonFromString(RawJson);

  Forwarder::ConfigSettings Settings = Config.extractConfiguration();

  ASSERT_EQ(1024u, Settings.ConversionWorkerQueueSize);
}

TEST(ConfigParserTest, extracting_conversion_worker_queue_size_sets_value) {
  std::string RawJson = R"({
                            "conversion-worker-queue-size": 1234
                           })";

  Forwarder::ConfigParser Config;
  Config.setJsonFromString(RawJson);

  Forwarder::ConfigSettings Settings = Config.extractConfiguration();

  ASSERT_EQ(1234u, Settings.ConversionWorkerQueueSize);
}

TEST(ConfigParserTest, no_main_poll_interval_sets_default) {
  std::string RawJson = "{}";

  Forwarder::ConfigParser Config;
  Config.setJsonFromString(RawJson);

  Forwarder::ConfigSettings Settings = Config.extractConfiguration();

  ASSERT_EQ(500, Settings.MainPollInterval);
}

TEST(ConfigParserTest, extracting_main_poll_interval_sets_value) {
  std::string RawJson = R"({
                            "main-poll-interval": 1234
                           })";

  Forwarder::ConfigParser Config;
  Config.setJsonFromString(RawJson);

  Forwarder::ConfigSettings Settings = Config.extractConfiguration();

  ASSERT_EQ(1234, Settings.MainPollInterval);
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
