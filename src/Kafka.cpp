#include "Kafka.h"
#include "logger.h"

namespace Forwarder {

static std::mutex mx;
static std::shared_ptr<InstanceSet> kset;

std::unique_lock<std::mutex> InstanceSet::getProducersByHostMutexLock() {
  std::unique_lock<std::mutex> lock(ProducersByHostMutex);
  return lock;
}

sptr<InstanceSet> InstanceSet::Set(KafkaW::BrokerSettings BrokerSettings) {
  std::lock_guard<std::mutex> lock(mx);
  LOG(Sev::Warning, "Kafka InstanceSet with rdkafka version: {}",
      RdKafka::version());
  if (!kset) {
    BrokerSettings.PollTimeoutMS = 0;
    kset.reset(new InstanceSet(BrokerSettings));
  }
  return kset;
}

void InstanceSet::clear() {
  std::lock_guard<std::mutex> lock(mx);
  kset.reset();
}

InstanceSet::InstanceSet(KafkaW::BrokerSettings BrokerSettings)
    : BrokerSettings(std::move(BrokerSettings)) {}

KafkaW::Producer::Topic InstanceSet::SetUpProducerTopic(Forwarder::URI uri) {
  LOG(Sev::Debug, "InstanceSet::producer_topic  for:  {}, {}", uri.host_port,
      uri.topic);
  auto host_port = uri.host_port;
  auto it = ProducersByHost.find(host_port);
  if (it != ProducersByHost.end()) {
    return KafkaW::Producer::Topic(it->second, uri.topic);
  }
  auto BrokerSettings = this->BrokerSettings;
  BrokerSettings.Address = host_port;
  auto Producer = std::make_shared<KafkaW::Producer>(BrokerSettings);
  {
    auto lock = getProducersByHostMutexLock();
    ProducersByHost[host_port] = Producer;
  }
  return KafkaW::Producer::Topic(Producer, uri.topic);
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
    LOG(Sev::Info, "Broker: {}  total: {}  outq: {}", m.first,
        Producer->TotalMessagesProduced, Producer->outputQueueLength());
  }
}

std::vector<KafkaW::ProducerStats> InstanceSet::getStatsForAllProducers() {
  std::vector<KafkaW::ProducerStats> ret;
  auto lock = getProducersByHostMutexLock();
  for (auto const &m : ProducersByHost) {
    ret.push_back(m.second->Stats);
  }
  return ret;
}
}
