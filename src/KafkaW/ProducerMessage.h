#pragma once
#include <stdint-gcc.h>

namespace KafkaW {
struct ProducerMsg {
  virtual ~ProducerMsg() = default;
  virtual void deliveryOk(){};
  virtual void deliveryError(){};
  unsigned char *data;
  uint32_t size;
};
}
