#include "fbschemas.h"
#include <flatbuffers/reflection.h>
#include "logger.h"

namespace BrightnESS {
namespace FlatBufs {

static_assert(FLATBUFFERS_LITTLEENDIAN, "We require little endian.");

/**
\class FB
\brief
Holds the flatbuffer until it has been sent.

Basically POD.  Holds the flatbuffer until no longer needed.
Also holds some internal testing data.
If you want to implement your own custom memory management, this is the
class to inherit from.
*/

/**
Gives a standard FlatBufferBuilder.
*/
FB::FB() :
		builder(new flatbuffers::FlatBufferBuilder())
{ }

/**
\brief FlatBufferBuilder with initial_size in bytes.
\param initial_size Initial size of the FlatBufferBuilder in bytes.
*/
FB::FB(uint32_t initial_size) :
		builder(new flatbuffers::FlatBufferBuilder(initial_size))
{ }

/**
\brief Your chance to implement your own memory recycling.
*/
FB::~FB() {
}

/**
Returns the underlying data of the flatbuffer.
Called when actually writing to Kafka.
*/
FBmsg FB::message() {
	if (!builder) {
		CLOG(8, 1, "builder no longer available");
		return {nullptr, 0};
	}
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
