#include "fbschemas.h"
#include <flatbuffers/reflection.h>
#include "logger.h"

namespace BrightnESS {
namespace FlatBufs {

#define NNE 2

fballoc::fballoc(FB * fb) : fb(fb) { }

uint8_t * fballoc::allocate(size_t size) const {
	//LOG(3, "Allocate new flat buffer: {} + {}", size, NNE);
	auto p1 = new uint8_t[size + NNE];
	//fmt::print("raw pointer at: {}\n", (void*)p1);
	p1 = p1 + NNE;
	return p1;
}

void fballoc::deallocate(uint8_t * p1) const {
	p1 = p1 - NNE;
	//LOG(3, "Deallocate {}", (void*)p1);
	delete[] p1;
}

FB::FB(Schema schema)
		: schema(schema),
			// yes, it's dirty..
			//header { *((uint8_t*)(&schema) + 0), *((uint8_t*)(&schema) + 1) },
			header {0x66, 0x77},
			alloc(decltype(alloc)(new fballoc(this))),
			builder(new flatbuffers::FlatBufferBuilder(2 * 1024 * 1024, alloc.get()))
{
	static_assert(FLATBUFFERS_LITTLEENDIAN, "Require little endian (would require little extra to cover big end as well)");
	alloc->fb = this;
}

std::pair<uint8_t *, size_t> FB::message() {
	auto ret = decltype(FB::message()) {
		builder->GetBufferPointer() - NNE,
		builder->GetSize() + NNE
	};
	// Put the header in place:
	for (int i1 = 0; i1 < NNE; ++i1) {
		*(ret.first+i1) = header[i1];
		//fmt::print("{:x}\n", *(ret.first+i1));
	}
	return ret;
}

void inspect(FB const & fb) {
}

}
}
