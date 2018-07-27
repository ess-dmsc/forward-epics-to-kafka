#include "FlatbufferMessage.h"
#include "logger.h"
#include <flatbuffers/reflection.h>

namespace FlatBufs {

static_assert(FLATBUFFERS_LITTLEENDIAN, "We require little endian.");

/// Constructs a standard FlatBufferBuilder.
FlatbufferMessage::FlatbufferMessage()
    : builder(new flatbuffers::FlatBufferBuilder()) {}

/// Constructs a FlatBufferBuilder with an initial size.
///
/// \param initial_size Initial size of the FlatBufferBuilder in bytes.
FlatbufferMessage::FlatbufferMessage(uint32_t initial_size)
    : builder(new flatbuffers::FlatBufferBuilder(initial_size)) {}

/// Destructor.
FlatbufferMessage::~FlatbufferMessage() {}

/// Returns the underlying data of the flatbuffer.
///
/// Called when actually writing to Kafka.
///
/// \return The underlying data.
FlatbufferMessageSlice FlatbufferMessage::message() {
  if (!builder) {
    CLOG(8, 1, "builder no longer available");
    return {nullptr, 0};
  }
  auto ret = decltype(FlatbufferMessage::message()){builder->GetBufferPointer(),
                                                    builder->GetSize()};
  return ret;
}

void inspect(FlatbufferMessage const &fb) {}
} // namespace FlatBufs
