// SPDX-License-Identifier: BSD-2-Clause
//
// This code has been produced by the European Spallation Source
// and its partner institutes under the BSD 2 Clause License.
//
// See LICENSE.md at the top level for license information.
//
// Screaming Udder!                              https://esss.se

#pragma once

#include <cstdint>
#include <cstdlib>

namespace KafkaW {

enum class PollStatus {
  Message,
  Error,
  EndOfPartition,
  Empty,
};

class ConsumerMessage {
public:
  ConsumerMessage(std::string &MessageData, PollStatus Status)
      : Data(MessageData), Status(Status) {}
  explicit ConsumerMessage(PollStatus Status) : Status(Status) {}
  std::string const getData() const { return Data; };
  PollStatus getStatus() const { return Status; }

private:
  std::string Data;
  PollStatus Status;
};
} // namespace KafkaW
