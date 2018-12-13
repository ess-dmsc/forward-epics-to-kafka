#pragma once

#include "ConfigParser.h"
#include "KafkaW/KafkaW.h"
#include "SchemaRegistry.h"
#include "URI.h"
#include <memory>
#include <string>
#include <vector>

namespace Forwarder {

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
  std::vector<char> Hostname;
  FlatBufs::SchemaRegistry schema_registry;
  KafkaW::BrokerSettings broker_opt;
  MainOpt();
  void set_broker(std::string &Broker);
  void init_logger();
  std::vector<StreamSettings> parseStreamsJson(const std::string &filepath);
};

std::pair<int, std::unique_ptr<MainOpt>> parse_opt(int argc, char **argv);

} // namespace Forwarder
