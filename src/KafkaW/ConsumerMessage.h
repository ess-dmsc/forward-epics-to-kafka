#pragma once

#include <cstdint>
#include <cstdlib>

namespace KafkaW {

enum class PollStatus { Message, Error, EndOfPartition, Empty, TimedOut };

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
}
