#pragma once
#include <stdint.h>
#include <string>

namespace KafkaW {
struct ProducerMessage {
  virtual ~ProducerMessage() = default;
  unsigned char *data;
  uint32_t size;
  std::string key;  // Used by producer if not empty
};
}
