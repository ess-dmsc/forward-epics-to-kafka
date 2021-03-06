// SPDX-License-Identifier: BSD-2-Clause
//
// This code has been produced by the European Spallation Source
// and its partner institutes under the BSD 2 Clause License.
//
// See LICENSE.md at the top level for license information.
//
// Screaming Udder!                              https://esss.se

#pragma once

#include "FlatbufferMessage.h"
#include "KafkaW/KafkaW.h"
#include <memory>

namespace Forwarder {

/// Represents the output sink used by Stream.
class KafkaOutput {
public:
  KafkaOutput(KafkaOutput &&) noexcept;
  explicit KafkaOutput(KafkaW::ProducerTopic &&OutputTopic);
  /// Hands off the message to Kafka
  int emit(std::unique_ptr<FlatBufs::FlatbufferMessage> fb);
  std::string topicName() const;
  KafkaW::ProducerTopic Output;
  SharedLogger Logger = getLogger();
};
} // namespace Forwarder
