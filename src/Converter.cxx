#include "Converter.h"
#include "logger.h"

namespace BrightnESS {
namespace ForwardEpicsToKafka {

Converter::sptr Converter::create(FlatBufs::SchemaRegistry const & schema_registry, std::string schema) {
	auto ret = Converter::sptr(new Converter);
	auto r1 = schema_registry.items().find(schema);
	if (r1 == schema_registry.items().end()) {
		LOG(3, "can not handle (yet?) schema id {}", schema);
		return nullptr;
	}
	ret->conv = r1->second->create_converter();
	return ret;
}

BrightnESS::FlatBufs::FB_uptr Converter::convert(FlatBufs::EpicsPVUpdate const & up) {
	return conv->convert(up);
}

}
}
