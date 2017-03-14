#include "fbschemas.h"
#include <flatbuffers/reflection.h>
#include "TopicMapping.h"
#include "logger.h"

namespace BrightnESS {
namespace FlatBufs {

static_assert(FLATBUFFERS_LITTLEENDIAN, "We require little endian.");

// Singleton
static fballoc g_fballoc;

fballoc::fballoc() { }

uint8_t * fballoc::allocate(size_t size) const {
	auto p1 = new uint8_t[size];
	if (!p1) {
		throw std::runtime_error("allocate failed");
	}
	return p1;
}

void fballoc::deallocate(uint8_t * p1) const {
	//LOG(4, "Deallocate {}", (void*)p1);
	delete[] p1;
}

FB::FB() :
		builder(new flatbuffers::FlatBufferBuilder())
{ }

FB::FB(uint32_t initial_size) :
		builder(new flatbuffers::FlatBufferBuilder(initial_size))
{ }

FB::FB(uint32_t initial_size, bool custom) :
		builder(new flatbuffers::FlatBufferBuilder(initial_size, &g_fballoc))
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
