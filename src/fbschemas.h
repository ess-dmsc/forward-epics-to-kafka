#pragma once

#include <memory>
#include <utility>
#include <flatbuffers/flatbuffers.h>
#include "KafkaW.h"

namespace BrightnESS {
namespace FlatBufs {


class FB;

class fballoc : public flatbuffers::simple_allocator {
public:
fballoc();
fballoc(int);
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

class FB : public KafkaW::ProducerMsg {
public:
FB();
FB(uint32_t initial_size);
FB(uint32_t initial_size, bool custom);
FBmsg message();
std::unique_ptr<flatbuffers::FlatBufferBuilder> builder;
// Used for performance measurements:
uint64_t seq = 0;
uint32_t fwdix = 0;
};
using FB_uptr = std::unique_ptr<FB>;

void inspect(FB const & fb);



}
}
