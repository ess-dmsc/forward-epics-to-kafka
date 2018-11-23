#include "FlatbufferMessage.h"
#include "logger.h"
#include <flatbuffers/reflection.h>

namespace FlatBufs {

static_assert(FLATBUFFERS_LITTLEENDIAN, "We require little endian.");

FlatbufferMessage::FlatbufferMessage()
    : builder(new flatbuffers::FlatBufferBuilder()) {}

FlatbufferMessage::FlatbufferMessage(uint32_t initial_size)
    : builder(new flatbuffers::FlatBufferBuilder(initial_size)) {}

std::pair<uint8_t *, size_t> FlatbufferMessage::message() {
  if (!builder) {
    LOG(Sev::Debug, "builder no longer available");
    return std::make_pair<uint8_t *, size_t>(nullptr, 0);
  }
  return std::make_pair<uint8_t *, size_t>(builder->GetBufferPointer(),
                                           builder->GetSize());
}
} // namespace FlatBufs
