// SPDX-License-Identifier: BSD-2-Clause
//
// This code has been produced by the European Spallation Source
// and its partner institutes under the BSD 2 Clause License.
//
// See LICENSE.md at the top level for license information.
//
// Screaming Udder!                              https://esss.se

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
  /// Constructor.
  ///
  /// \param BrokerSettings The "global" Kafka settings for the data forwarding
  /// producers.
  explicit InstanceSet(KafkaW::BrokerSettings BrokerSettings);

  /// Create a producer topic.
  ///
  /// Note: will return the existing producer topic if it already exists.
  ///
  /// \param Uri The broker URI.
  /// \return The associated producer topic.
  KafkaW::ProducerTopic createProducerTopic(URI const &Uri);

  /// Poll all the producers.
  void poll();

  /// Log the stats for all the producers.
  void logStats();

  /// Get the stats for the producers.
  ///
  /// \return The producer stats.
  std::vector<KafkaW::ProducerStats> getStatsForAllProducers();

private:
  /// Contains the general Kafka settings, e.g. timeouts, message sizes etc.
  /// Does not contain the Broker addresses, these are held in the individual
  /// producers.
  KafkaW::BrokerSettings BrokerSettings;
  std::map<std::string, std::shared_ptr<KafkaW::Producer>> ProducersByHost;
  std::mutex ProducersMutex;
  SharedLogger Logger = getLogger();
};
} // namespace Forwarder
