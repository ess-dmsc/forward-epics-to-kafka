#pragma once

/**
\file
Manage the running Kafka producer instances.
Simple load balance over the available producers.
*/

#include <memory>
#include <atomic>
#include <mutex>
#include <thread>
#include <vector>
#include <map>
#include <string>

#include "tools.h"
#include "uri.h"

#include <librdkafka/rdkafka.h>
#include "fbhelper.h"
#include "fbschemas.h"
#include "KafkaW.h"

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
  std::map<std::string, std::shared_ptr<KafkaW::Producer> > producers_by_host;
};
}
}
}
