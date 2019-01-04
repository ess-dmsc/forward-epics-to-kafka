#pragma once

/// \file
/// Manage the running Kafka producer instances.
/// Simple load balance over the available producers.

#include "URI.h"
#include <atomic>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "FlatbufferMessage.h"
#include "KafkaW/KafkaW.h"

namespace Forwarder {

template <typename T> using sptr = std::shared_ptr<T>;

class InstanceSet {
public:
  static sptr<InstanceSet> Set(KafkaW::BrokerSettings Settings);
  static void clear();
  KafkaW::Producer::Topic SetUpProducerTopic(URI uri);
  int poll();
  void log_stats();
  std::vector<KafkaW::ProducerStats> getStatsForAllProducers();
  InstanceSet(InstanceSet const &&) = delete;

private:
  explicit InstanceSet(KafkaW::BrokerSettings opt);
  std::unique_lock<std::mutex> getProducersByHostMutexLock();
  KafkaW::BrokerSettings BrokerSettings;
  std::mutex ProducersByHostMutex;
  std::map<std::string, std::shared_ptr<KafkaW::Producer>> ProducersByHost;
};
} // namespace Forwarder
