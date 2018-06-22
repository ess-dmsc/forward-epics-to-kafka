#pragma once

#include "ConfigParser.h"
#include "KafkaW/KafkaW.h"
#include "SchemaRegistry.h"
#include "uri.h"
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
  std::string ConfigurationFile;
  uint32_t PeriodMS = 0;
  uint32_t FakePVPeriodMS = 0;
  uint64_t teamid = 0;
  std::vector<char> Hostname;
  FlatBufs::SchemaRegistry schema_registry;
  KafkaW::BrokerSettings broker_opt;
  void parse_json_file(std::string ConfigurationFile);
  MainOpt();
  void set_broker(std::string &Broker);
  void init_logger();

private:
  ConfigSettings parse_document(const std::string &filepath);
};

std::pair<int, std::unique_ptr<MainOpt>> parse_opt(int argc, char **argv);

} // namespace Forwarder
