// SPDX-License-Identifier: BSD-2-Clause
//
// This code has been produced by the European Spallation Source
// and its partner institutes under the BSD 2 Clause License.
//
// See LICENSE.md at the top level for license information.
//
// Screaming Udder!                              https://esss.se

#include "Kafka.h"
#include "logger.h"
#include <algorithm>

namespace Forwarder {

InstanceSet::InstanceSet(KafkaW::BrokerSettings BrokerSettings)
    : BrokerSettings(std::move(BrokerSettings)) {}

KafkaW::ProducerTopic
InstanceSet::createProducerTopic(Forwarder::URI const &Uri) {
  Logger->debug("InstanceSet::producer_topic for: {}, {}", Uri.HostPort,
                Uri.Topic);
  auto it = ProducersByHost.find(Uri.HostPort);
  if (it != ProducersByHost.end()) {
    return KafkaW::ProducerTopic(it->second, Uri.Topic);
  }

  // Copy the global settings and set the specific address
  auto BrokerSettingsCopy = this->BrokerSettings;
  BrokerSettingsCopy.Address = Uri.HostPort;
  auto Producer = std::make_shared<KafkaW::Producer>(BrokerSettingsCopy);
  {
    std::lock_guard<std::mutex> UpdateProducersLock(ProducersMutex);
    ProducersByHost[Uri.HostPort] = Producer;
  }
  return KafkaW::ProducerTopic(Producer, Uri.Topic);
}

void InstanceSet::poll() {
  std::lock_guard<std::mutex> PollProducersLock(ProducersMutex);
  for (auto const &ProducerMap : ProducersByHost) {
    auto &Producer = ProducerMap.second;
    Producer->poll();
  }
}

void InstanceSet::logMetrics() {
  std::lock_guard<std::mutex> QueryProducersLock(ProducersMutex);
  for (auto const &m : ProducersByHost) {
    auto &Producer = m.second;
    Logger->info("Broker: {}  total: {}  outq: {}", m.first,
                 Producer->TotalMessagesProduced,
                 Producer->outputQueueLength());
  }
}

std::vector<KafkaW::ProducerStats> InstanceSet::getStatsForAllProducers() {
  std::lock_guard<std::mutex> QueryProducersLock(ProducersMutex);
  std::vector<KafkaW::ProducerStats> ret;
  std::transform(
      ProducersByHost.cbegin(), ProducersByHost.cend(), std::back_inserter(ret),
      [](const std::pair<std::string, std::shared_ptr<KafkaW::Producer>>
             &CProducer) { return CProducer.second->Stats; });
  return ret;
}
} // namespace Forwarder
