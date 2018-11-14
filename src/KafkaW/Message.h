#pragma once

#include <cstdint>
#include <cstdlib>

namespace KafkaW {

enum PollStatus {
  Msg,
  Err,
  EOP,
  Empty,
};

class Message {
public:
  Message(std::uint8_t const *Pointer, size_t Size, PollStatus Status) : DataPointer(Pointer), DataSize(Size), Status(Status){}
  explicit Message(PollStatus Status) : Status(Status) {}
  std::uint8_t const *getData() const { return DataPointer; };
  PollStatus getStatus() {return Status;}
  size_t getSize() {return DataSize; }

private:
  unsigned char const *DataPointer{nullptr};
  size_t DataSize{0};
  PollStatus Status;
};
}
