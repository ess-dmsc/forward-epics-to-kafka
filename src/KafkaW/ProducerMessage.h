#pragma once
#include <stdint.h>

namespace KafkaW {
struct ProducerMessage {
  virtual ~ProducerMessage() = default;
  virtual void deliveryOk(){};
  virtual void deliveryError(){};
  unsigned char *data;
  uint32_t size;
};
}
