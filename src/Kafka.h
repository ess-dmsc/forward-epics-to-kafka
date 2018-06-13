#pragma once

/**
\file
Manage the running Kafka producer instances.
Simple load balance over the available producers.
*/

#include "uri.h"
#include <atomic>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "FlatbufferMessage.h"
#include "KafkaW/KafkaW.h"
#include <librdkafka/rdkafka.h>

namespace Forwarder {

template <typename T> using sptr = std::shared_ptr<T>;

class InstanceSet {
public:
  static sptr<InstanceSet> Set(KafkaW::BrokerSettings opt);
  static void clear();
  KafkaW::Producer::Topic producer_topic(URI uri);
  int poll();
  void log_stats();
  std::vector<KafkaW::ProducerStats> stats_all();

private:
  InstanceSet(InstanceSet const &&) = delete;
  InstanceSet(KafkaW::BrokerSettings opt);
  KafkaW::BrokerSettings BrokerSettings;
  std::mutex mx_producers_by_host;
  std::map<std::string, std::shared_ptr<KafkaW::Producer>> producers_by_host;
};
}
