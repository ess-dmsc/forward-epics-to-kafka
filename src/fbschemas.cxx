#include "fbschemas.h"
#include <flatbuffers/reflection.h>
#include "TopicMapping.h"
#include "logger.h"

namespace BrightnESS {
namespace FlatBufs {

static_assert(FLATBUFFERS_LITTLEENDIAN, "We require little endian.");

FB::FB() :
		builder(new flatbuffers::FlatBufferBuilder())
{ }

FB::FB(uint32_t initial_size) :
		builder(new flatbuffers::FlatBufferBuilder(initial_size))
{ }

FBmsg FB::message() {
	auto ret = decltype(FB::message()) {
		builder->GetBufferPointer(),
		builder->GetSize()
	};
	return ret;
}

void inspect(FB const & fb) {
}

}
}
