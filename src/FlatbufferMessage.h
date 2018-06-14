#pragma once

#include "FlatbufferMessageSlice.h"
#include "KafkaW/KafkaW.h"
#include <flatbuffers/flatbuffers.h>
#include <memory>
#include <utility>

namespace FlatBufs {

/// Forward declarations for friending.

namespace f142 {
class Converter;
class ConverterTestNamed;
} // namespace f142

/// \brief
/// Holds the flatbuffer until it has been sent.
///
/// Basically POD.  Holds the flatbuffer until no longer needed.
/// Also holds some internal counters for performance testing.
/// If you want to implement your own custom memory management, this is the
/// class to inherit from.

class FlatbufferMessage : public KafkaW::Producer::Msg {
public:
  using uptr = std::unique_ptr<FlatbufferMessage>;
  FlatbufferMessage();
  explicit FlatbufferMessage(uint32_t initial_size);
  ~FlatbufferMessage() override;
  FlatbufferMessageSlice message();
  std::unique_ptr<flatbuffers::FlatBufferBuilder> builder;
  FlatbufferMessage(FlatbufferMessage const &) = delete;

private:
  // Used for performance tests, please do not touch.
  uint64_t seq = 0;
  friend class Kafka;
  // Only here for some specific tests:
  friend class f142::Converter;
  friend class f142::ConverterTestNamed;
};

} // namespace FlatBufs
