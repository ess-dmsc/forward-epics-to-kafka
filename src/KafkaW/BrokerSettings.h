#pragma once

#include <map>
#include <string>

struct rd_kafka_conf_s;
typedef struct rd_kafka_conf_s rd_kafka_conf_t;

namespace KafkaW {

/// Collect options used to connect to the broker.

struct BrokerSettings {
  BrokerSettings();
  void apply(rd_kafka_conf_t *RdKafkaConfiguration);
  std::string Address;
  int PollTimeoutMS = 100;
  std::map<std::string, int64_t> ConfigurationIntegers;
  std::map<std::string, std::string> ConfigurationStrings;
};
}
