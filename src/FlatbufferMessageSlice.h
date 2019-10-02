// SPDX-License-Identifier: BSD-2-Clause
//
// This code has been produced by the European Spallation Source
// and its partner institutes under the BSD 2 Clause License.
//
// See LICENSE.md at the top level for license information.
//
// Screaming Udder!                              https://esss.se

#pragma once

#include <cstddef>
#include <cstdint>

namespace FlatBufs {

/// A view into a FlatbufferMessage slice.

struct FlatbufferMessageSlice {
  uint8_t *data;
  size_t size;
};
} // namespace FlatBufs
