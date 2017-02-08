#pragma once

#include <flatbuffers/flatbuffers.h>

/** \file
Helper definitions for working with flat buffers.
*/

namespace BrightnESS {
namespace ForwardEpicsToKafka {
namespace Epics {
// In this namespace for historical reasons...
using FBBptr = std::unique_ptr<flatbuffers::FlatBufferBuilder>;
}
}
}
