#include "Kafka.h"
#include "logger.h"
#include <algorithm>

namespace Forwarder {

static std::mutex ProducerMutex;
static std::shared_ptr<InstanceSet> kset;

std::unique_lock<std::mutex> InstanceSet::getProducersByHostMutexLock() {
  std::unique_lock<std::mutex> lock(ProducersByHostMutex);
  return lock;
}

std::shared_ptr<InstanceSet>
InstanceSet::Set(KafkaW::BrokerSettings BrokerSettings) {
  std::lock_guard<std::mutex> lock(ProducerMutex);
  getLogger()->warn("Kafka InstanceSet with rdkafka version: {}",
                    RdKafka::version());
  if (!kset) {
    BrokerSettings.PollTimeoutMS = 0;
    kset.reset(new InstanceSet(BrokerSettings));
  }
  return kset;
}

void InstanceSet::clear() {
  std::lock_guard<std::mutex> lock(ProducerMutex);
  kset.reset();
}

InstanceSet::InstanceSet(KafkaW::BrokerSettings BrokerSettings)
    : BrokerSettings(std::move(BrokerSettings)) {}

KafkaW::ProducerTopic InstanceSet::SetUpProducerTopic(Forwarder::URI uri) {
  Logger->debug("InstanceSet::producer_topic  for:  {}, {}", uri.HostPort,
                uri.Topic);
  auto host_port = uri.HostPort;
  auto it = ProducersByHost.find(host_port);
  if (it != ProducersByHost.end()) {
    return KafkaW::ProducerTopic(it->second, uri.Topic);
  }
  auto BrokerSettings = this->BrokerSettings;
  BrokerSettings.Address = host_port;
  auto Producer = std::make_shared<KafkaW::Producer>(BrokerSettings);
  {
    auto lock = getProducersByHostMutexLock();
    ProducersByHost[host_port] = Producer;
  }
  return KafkaW::ProducerTopic(Producer, uri.Topic);
}

int InstanceSet::poll() {
  auto lock = getProducersByHostMutexLock();
  for (auto const &ProducerMap : ProducersByHost) {
    auto &Producer = ProducerMap.second;
    Producer->poll();
  }
  return 0;
}

void InstanceSet::log_stats() {
  auto lock = getProducersByHostMutexLock();
  for (auto const &m : ProducersByHost) {
    auto &Producer = m.second;
    Logger->info("Broker: {}  total: {}  outq: {}", m.first,
                 Producer->TotalMessagesProduced,
                 Producer->outputQueueLength());
  }
}

std::vector<KafkaW::ProducerStats> InstanceSet::getStatsForAllProducers() {
  std::vector<KafkaW::ProducerStats> ret;
  auto lock = getProducersByHostMutexLock();
  std::transform(
      ProducersByHost.cbegin(), ProducersByHost.cend(), std::back_inserter(ret),
      [](const std::pair<std::string, std::shared_ptr<KafkaW::Producer>>
             &CProducer) { return CProducer.second->Stats; });
  return ret;
}
}
