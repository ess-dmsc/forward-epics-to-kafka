#pragma once

#include <memory>
#include <utility>
#include "simple_generated.h"
#include "general_generated.h"

namespace BrightnESS {
namespace FlatBufs {

enum class Schema: uint16_t {
	General = 0xf1,
	Simple = 0x1f
};

class FB;

class fballoc : public flatbuffers::simple_allocator {
public:
fballoc(FB * fb);
uint8_t * allocate(size_t size) const override;
void deallocate(uint8_t * p) const override;
~fballoc() { }
FB * fb;
};

class FB {
public:
FB(Schema schema);
std::pair<uint8_t *, size_t> message();
// Internal identifier
Schema schema;
uint8_t header[2] = {0xaa, 0xbb};
std::unique_ptr<fballoc> alloc;
std::unique_ptr<flatbuffers::FlatBufferBuilder> builder;
};
using FB_uptr = std::unique_ptr<FB>;

void inspect(FB const & fb);

}
}
