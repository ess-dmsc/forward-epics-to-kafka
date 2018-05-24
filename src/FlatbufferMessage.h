#pragma once

#include "FlatbufferMessageSlice.h"
#include "KafkaW/KafkaW.h"
#include <flatbuffers/flatbuffers.h>
#include <memory>
#include <utility>

namespace BrightnESS {
namespace FlatBufs {

/// Forward declarations for friending.

namespace f142 {
class Converter;
class ConverterTestNamed;
}

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

  /// Gives a standard FlatBufferBuilder.
  FlatbufferMessage() : builder(new flatbuffers::FlatBufferBuilder()){};

  /// \brief FlatBufferBuilder with initial_size in bytes.
  /// \param initial_size Initial size of the FlatBufferBuilder in bytes.
  FlatbufferMessage(uint32_t initial_size)
      : builder(new flatbuffers::FlatBufferBuilder(initial_size)){};

  /// \brief Your chance to implement your own memory recycling.
  ~FlatbufferMessage() override{};

  FlatbufferMessage(FlatbufferMessage const &) = delete;
  FlatbufferMessageSlice message();
  std::unique_ptr<flatbuffers::FlatBufferBuilder> builder;

private:
  friend class Kafka;
  // Only here for some specific tests:
  friend class f142::Converter;
  friend class f142::ConverterTestNamed;
};
}
}
