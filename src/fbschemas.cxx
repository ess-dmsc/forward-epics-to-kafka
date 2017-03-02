#include "fbschemas.h"
#include <flatbuffers/reflection.h>
#include "TopicMapping.h"
#include "logger.h"

namespace BrightnESS {
namespace FlatBufs {

#define NNE 0

// Singleton
static fballoc g_fballoc;

fballoc::fballoc() { }

uint8_t * fballoc::allocate(size_t size) const {
	//LOG(4, "Allocate new flat buffer: {} + {}", size, NNE);
	auto p1 = new uint8_t[size + NNE];
	//fmt::print("raw pointer at: {}\n", (void*)p1);
	p1 = p1 + NNE;
	return p1;
}

void fballoc::deallocate(uint8_t * p1) const {
	p1 = p1 - NNE;
	//LOG(4, "Deallocate {}", (void*)p1);
	delete[] p1;
}

FB::FB()
		: builder(new flatbuffers::FlatBufferBuilder(2 * 1024 * 1024, &g_fballoc))
{
	static_assert(FLATBUFFERS_LITTLEENDIAN, "Ctor requires little endian (would require little extra to cover big end as well)");
}

FBmsg FB::message() {
	auto ret = decltype(FB::message()) {
		builder->GetBufferPointer() - NNE,
		builder->GetSize() + NNE
	};
	return ret;
}

void inspect(FB const & fb) {
}

#undef NNE

}
}
