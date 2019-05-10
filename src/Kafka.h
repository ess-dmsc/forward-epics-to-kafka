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

class InstanceSet {
public:
  static std::shared_ptr<InstanceSet>
  Set(KafkaW::BrokerSettings BrokerSettings);
  static void clear();
  KafkaW::ProducerTopic SetUpProducerTopic(URI uri);
  int poll();
  void log_stats();
  std::vector<KafkaW::ProducerStats> getStatsForAllProducers();
  InstanceSet(InstanceSet const &&) = delete;

private:
  explicit InstanceSet(KafkaW::BrokerSettings BrokerSettings);
  std::unique_lock<std::mutex> getProducersByHostMutexLock();
  KafkaW::BrokerSettings BrokerSettings;
  std::mutex ProducersByHostMutex;
  std::map<std::string, std::shared_ptr<KafkaW::Producer>> ProducersByHost;
  SharedLogger Logger = getLogger();
};
} // namespace Forwarder
