#pragma once

/**
\file
Manage the running Kafka producer instances.
Simple load balance over the available producers.
*/

#include <atomic>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "tools.h"
#include "uri.h"

#include "KafkaW.h"
#include "fbhelper.h"
#include "fbschemas.h"
#include <librdkafka/rdkafka.h>

namespace BrightnESS {
namespace ForwardEpicsToKafka {
namespace Kafka {

template <typename T> using sptr = std::shared_ptr<T>;

class InstanceSet {
public:
  static sptr<InstanceSet> Set(KafkaW::BrokerOpt opt);
  KafkaW::Producer::Topic producer_topic(uri::URI uri);
  int poll();
  void log_stats();
  std::vector<KafkaW::Producer::Stats> stats_all();

private:
  InstanceSet(InstanceSet const &&) = delete;
  InstanceSet(KafkaW::BrokerOpt opt);
  KafkaW::BrokerOpt opt;
  std::mutex mx_producers_by_host;
  std::map<std::string, std::shared_ptr<KafkaW::Producer>> producers_by_host;
};
}
}
}
