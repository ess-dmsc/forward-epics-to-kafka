#pragma once

#include <deque>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include "uri.h"

namespace BrightnESS {
namespace ForwardEpicsToKafka {

class Configuration {
public:
  explicit Configuration(std::string RawJson);
  void extractBrokerConfig();
  void extractBrokers();
  uri::URI BrokerConfig{"//localhost:9092/forward_epics_to_kafka_commands"};
  std::vector<uri::URI> Brokers;

private:
  nlohmann::json Json;
  void setBrokers(std::string Brokers);
};
}
}
