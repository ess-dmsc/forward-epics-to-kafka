#include "FlatbufferMessage.h"
#include "logger.h"
#include <flatbuffers/reflection.h>

namespace FlatBufs {

static_assert(FLATBUFFERS_LITTLEENDIAN, "We require little endian.");

FlatbufferMessage::FlatbufferMessage()
    : builder(new flatbuffers::FlatBufferBuilder()) {}

FlatbufferMessage::FlatbufferMessage(uint32_t initial_size)
    : builder(new flatbuffers::FlatBufferBuilder(initial_size)) {}

FlatbufferMessageSlice FlatbufferMessage::message() {
  if (!builder) {
    spdlog::get("ForwarderLogger")->trace("builder no longer available");
    return {nullptr, 0};
  }
  auto ret = decltype(FlatbufferMessage::message()){builder->GetBufferPointer(),
                                                    builder->GetSize()};
  return ret;
}
} // namespace FlatBufs
