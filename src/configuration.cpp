#include "configuration.h"
#include "helper.h"
#include "json.h"
#include <iostream>

namespace BrightnESS {
namespace ForwardEpicsToKafka {

Configuration::Configuration(std::string RawJson) {
  Json = nlohmann::json::parse(RawJson);
}

void Configuration::extractBrokerConfig() {
  if (auto x = find<std::string>("broker-config", Json)) {
    BrokerConfig = x.inner();
  }
}

void Configuration::extractBrokers() {
  if (auto x = find<std::string>("broker", Json)) {
    setBrokers(x.inner());
  }
}

void Configuration::setBrokers(std::string Broker) {
  Brokers.clear();
  auto a = split(Broker, ",");
  for (auto &x : a) {
    uri::URI u1;
    u1.require_host_slashes = false;
    u1.parse(x);
    Brokers.push_back(u1);
  }
}

}
}
