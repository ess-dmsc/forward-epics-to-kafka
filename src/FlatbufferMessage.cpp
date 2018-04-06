#include "FlatbufferMessage.h"
#include "logger.h"
#include <flatbuffers/reflection.h>

namespace BrightnESS {
namespace FlatBufs {

static_assert(FLATBUFFERS_LITTLEENDIAN, "We require little endian.");

/// \class FB
/// \brief
/// Holds the flatbuffer until it has been sent.
///
/// Basically POD.  Holds the flatbuffer until no longer needed.
/// Also holds some internal testing data.
/// If you want to implement your own custom memory management, this is the
/// class to inherit from.

/// Gives a standard FlatBufferBuilder.

FlatbufferMessage::FlatbufferMessage()
    : builder(new flatbuffers::FlatBufferBuilder()) {}

/// \brief FlatBufferBuilder with initial_size in bytes.
/// \param initial_size Initial size of the FlatBufferBuilder in bytes.

FlatbufferMessage::FlatbufferMessage(uint32_t initial_size)
    : builder(new flatbuffers::FlatBufferBuilder(initial_size)) {}

/// \brief Your chance to implement your own memory recycling.

FlatbufferMessage::~FlatbufferMessage() {}

/// Returns the underlying data of the flatbuffer.
/// Called when actually writing to Kafka.

FlatbufferRawMessageSlice FlatbufferMessage::message() {
  if (!builder) {
    CLOG(8, 1, "builder no longer available");
    return {nullptr, 0};
  }
  auto ret = decltype(FlatbufferMessage::message()){builder->GetBufferPointer(),
                                                    builder->GetSize()};
  return ret;
}

void inspect(FlatbufferMessage const &fb) {}
}
}
