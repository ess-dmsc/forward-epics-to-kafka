#include "epics-to-fb.h"

namespace BrightnESS {
namespace FlatBufs {

std::map<std::string, SchemaInfo::ptr> & SchemaRegistry::items() {
	static std::map<std::string, SchemaInfo::ptr> _items;
	return _items;
}

}
}
