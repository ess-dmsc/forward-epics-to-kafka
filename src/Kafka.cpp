#include "Kafka.h"
#include "logger.h"

namespace Forwarder {

static std::mutex mx;
static std::shared_ptr<InstanceSet> kset;

sptr<InstanceSet> InstanceSet::Set(KafkaW::BrokerSettings BrokerSettings) {
  std::unique_lock<std::mutex> lock(mx);
  LOG(4, "Kafka InstanceSet with rdkafka version: {}", rd_kafka_version_str());
  if (!kset) {
    BrokerSettings.PollTimeoutMS = 0;
    kset.reset(new InstanceSet(BrokerSettings));
  }
  return kset;
}

void InstanceSet::clear() {
  std::unique_lock<std::mutex> lock(mx);
  kset.reset();
}

InstanceSet::InstanceSet(KafkaW::BrokerSettings BrokerSettings)
    : BrokerSettings(BrokerSettings) {}

static void prod_delivery_ok(rd_kafka_message_t const *msg) {
  if (auto x = msg->_private) {
    auto p = static_cast<KafkaW::Producer::Msg *>(x);
    p->deliveryOk();
    delete p;
  }
}

static void prod_delivery_failed(rd_kafka_message_t const *msg) {
  if (auto x = msg->_private) {
    auto p = static_cast<KafkaW::Producer::Msg *>(x);
    p->deliveryError();
    delete p;
  }
}

KafkaW::Producer::Topic InstanceSet::producer_topic(Forwarder::URI uri) {
  LOG(7, "InstanceSet::producer_topic  for:  {}, {}", uri.host_port, uri.topic);
  auto host_port = uri.host_port;
  auto it = producers_by_host.find(host_port);
  if (it != producers_by_host.end()) {
    return KafkaW::Producer::Topic(it->second, uri.topic);
  }
  auto BrokerSettings = this->BrokerSettings;
  BrokerSettings.Address = host_port;
  auto p = std::make_shared<KafkaW::Producer>(BrokerSettings);
  p->on_delivery_ok = prod_delivery_ok;
  p->on_delivery_failed = prod_delivery_failed;
  {
    std::unique_lock<std::mutex> lock(mx_producers_by_host);
    producers_by_host[host_port] = p;
  }
  return KafkaW::Producer::Topic(p, uri.topic);
}

int InstanceSet::poll() {
  std::unique_lock<std::mutex> lock(mx_producers_by_host);
  for (auto const &m : producers_by_host) {
    auto &p = m.second;
    p->poll();
  }
  return 0;
}

void InstanceSet::log_stats() {
  std::unique_lock<std::mutex> lock(mx_producers_by_host);
  for (auto const &m : producers_by_host) {
    auto &p = m.second;
    LOG(6, "Broker: {}  total: {}  outq: {}", m.first, p->TotalMessagesProduced,
        p->outputQueueLength());
  }
}

std::vector<KafkaW::ProducerStats> InstanceSet::stats_all() {
  std::vector<KafkaW::ProducerStats> ret;
  std::unique_lock<std::mutex> lock(mx_producers_by_host);
  for (auto const &m : producers_by_host) {
    ret.push_back(m.second->Stats);
  }
  return ret;
}
}
