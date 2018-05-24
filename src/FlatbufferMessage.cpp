#include "FlatbufferMessage.h"
#include "logger.h"
#include <flatbuffers/reflection.h>

namespace BrightnESS {
namespace FlatBufs {

static_assert(FLATBUFFERS_LITTLEENDIAN, "We require little endian.");

/// Returns the underlying data of the flatbuffer.
/// Called when actually writing to Kafka.

FlatbufferMessageSlice FlatbufferMessage::message() {
  if (!builder) {
    CLOG(8, 1, "builder no longer available");
    return {nullptr, 0};
  }
  auto ret = decltype(FlatbufferMessage::message()){builder->GetBufferPointer(),
                                                    builder->GetSize()};
  return ret;
}
}
}
