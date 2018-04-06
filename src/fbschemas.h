#pragma once

#include "KafkaW/KafkaW.h"
#include <flatbuffers/flatbuffers.h>
#include <memory>
#include <utility>

namespace BrightnESS {
namespace FlatBufs {

/// A raw view into a Flatbuffer

struct FlatbufferRawMessageSlice {
  uint8_t *data;
  size_t size;
};

/// Forward declarations for friending.

namespace f140 {
class Converter;
}
namespace f141 {
class Converter;
}
namespace f142 {
class Converter;
}
namespace f142 {
class ConverterTestNamed;
}

class FlatbufferMessage : public KafkaW::Producer::Msg {
public:
  using uptr = std::unique_ptr<FlatbufferMessage>;
  FlatbufferMessage();
  FlatbufferMessage(uint32_t initial_size);
  ~FlatbufferMessage() override;
  FlatbufferRawMessageSlice message();
  std::unique_ptr<flatbuffers::FlatBufferBuilder> builder;

private:
  FlatbufferMessage(FlatbufferMessage const &) = delete;
  // Used for performance tests, please do not touch.
  uint64_t seq = 0;
  uint32_t fwdix = 0;
  friend class Kafka;
  // Only here for some specific tests:
  friend class f142::ConverterTestNamed;
  friend class f140::Converter;
  friend class f141::Converter;
  friend class f142::Converter;
};

void inspect(FlatbufferMessage const &fb);
}
}
