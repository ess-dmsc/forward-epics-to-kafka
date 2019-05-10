#pragma once

#include "FlatbufferMessageSlice.h"
#include "KafkaW/ProducerMessage.h"
#include "logger.h"
#include <flatbuffers/flatbuffers.h>
#include <memory>
#include <utility>

namespace FlatBufs {

/// Forward declarations for friends.

namespace f142 {
class Converter;
} // namespace f142

/// Holds the flatbuffer until it has been sent.
///
/// Basically POD.  Holds the flatbuffer until no longer needed.
/// If you want to implement your own custom memory management, this is the
/// class to inherit from.
class FlatbufferMessage : public KafkaW::ProducerMessage {
public:
  /// Constructs a standard FlatBufferBuilder.
  FlatbufferMessage();
  /// Constructs a FlatBufferBuilder with an initial size.
  ///
  /// \param initial_size Initial size of the FlatBufferBuilder in bytes.
  explicit FlatbufferMessage(uint32_t initial_size);

  /// Destructor.
  ~FlatbufferMessage() override = default;

  /// Returns the underlying data of the flatbuffer.
  ///
  /// Called when actually writing to Kafka.
  ///
  /// \return The underlying data.
  FlatbufferMessageSlice message();

  std::unique_ptr<flatbuffers::FlatBufferBuilder> builder;
  FlatbufferMessage(FlatbufferMessage const &) = delete;

private:
  SharedLogger Logger = getLogger();
};
} // namespace FlatBufs
