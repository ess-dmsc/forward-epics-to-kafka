#pragma once

#include "ConfigParser.h"
#include "KafkaW/BrokerSettings.h"
#include "SchemaRegistry.h"
#include "URI.h"
#include <memory>
#include <string>
#include <vector>

namespace Forwarder {

std::vector<StreamSettings> parseStreamsJson(const std::string &filepath);

struct MainOpt {
  ConfigSettings MainSettings;
  std::string brokers_as_comma_list() const;
  std::string KafkaGELFAddress = "";
  std::string GraylogLoggerAddress = "";
  std::string InfluxURI = "";
  std::string LogFilename;
  std::string StreamsFile;
  uint32_t PeriodMS = 0;
  uint32_t FakePVPeriodMS = 0;
  spdlog::level::level_enum LoggingLevel;
  std::vector<char> Hostname;
  FlatBufs::SchemaRegistry schema_registry;
  KafkaW::BrokerSettings broker_opt;
  MainOpt();
  void init_logger();
};

std::pair<int, std::unique_ptr<MainOpt>> parse_opt(int argc, char **argv);

} // namespace Forwarder
