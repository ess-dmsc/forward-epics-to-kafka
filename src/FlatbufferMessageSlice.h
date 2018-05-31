#pragma once

#include <cstddef>
#include <cstdint>

namespace FlatBufs {

/// A view into a FlatbufferMessage slice.

struct FlatbufferMessageSlice {
  uint8_t *data;
  size_t size;
};
}
