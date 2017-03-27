#pragma once

#include <memory>
#include <utility>
#include <flatbuffers/flatbuffers.h>
#include "KafkaW.h"

namespace BrightnESS {
namespace FlatBufs {

// POD
class FBmsg {
public:
uint8_t * data;
size_t size;
};

namespace f140 { class Converter; }
namespace f141 { class Converter; }
namespace f142 { class Converter; }
namespace f142 { class ConverterTestNamed; }

class FB : public KafkaW::Producer::Msg {
public:
FB();
FB(uint32_t initial_size);
~FB() override;
FBmsg message();
std::unique_ptr<flatbuffers::FlatBufferBuilder> builder;
private:
FB(FB const &) = delete;
// Used for performance tests, please do not touch.
uint64_t seq = 0;
uint32_t fwdix = 0;
friend class Kafka;
// Only here for some specific tests:
friend class f142::ConverterTestNamed;
friend class f140::Converter;
friend class f141::Converter;
friend class f142::Converter;
};
using FB_uptr = std::unique_ptr<FB>;

void inspect(FB const & fb);

}
}
