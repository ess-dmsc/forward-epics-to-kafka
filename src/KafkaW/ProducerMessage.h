#pragma once
#include <stdint.h>
#include <string>

namespace KafkaW {
struct ProducerMessage {
  virtual ~ProducerMessage() = default;
  unsigned char *Data;
  uint32_t Size;
  std::string Key; // Used by producer if not empty
};
}
