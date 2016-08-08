#pragma once

/** \file
Some helper definitions for working with the flat buffers.
*/

#include "simple_generated.h"

namespace BrightnESS {
namespace ForwardEpicsToKafka {
namespace Epics {

using FBBptr = std::unique_ptr<flatbuffers::FlatBufferBuilder>;

}
}
}
