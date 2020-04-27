#pragma once

#include <KafkaW/Producer.h>
#include <trompeloeil.hpp>

namespace KafkaW {

class MockProducer : public Producer {
public:
  using Producer::Producer;
  MAKE_MOCK0(poll, void(), override);
  MAKE_MOCK0(outputQueueLength, int(), override);
  MAKE_MOCK8(produce,
             RdKafka::ErrorCode(RdKafka::Topic *, int32_t, int, void *, size_t,
                                const void *, size_t, void *),
             override);
};

} // namespace KafkaW
