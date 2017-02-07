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
fballoc();
uint8_t * allocate(size_t size) const override;
void deallocate(uint8_t * p) const override;
~fballoc() { }
};

// POD
class FBmsg {
public:
uint8_t * data;
size_t size;
};

class FB {
public:
FB(Schema schema);
//void finalize();
FBmsg message();
// Internal identifier
Schema schema;
std::unique_ptr<flatbuffers::FlatBufferBuilder> builder;
// Used for performance measurements:
uint64_t seq = 0;
uint8_t fwdix = 0;
};
using FB_uptr = std::unique_ptr<FB>;

void inspect(FB const & fb);

}
}
