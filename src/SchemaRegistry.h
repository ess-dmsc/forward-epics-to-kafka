#pragma once
#include <memory>
#include <map>
#include <string>
#include "MakeFlatBufferFromPVStructure.h"

namespace BrightnESS {
namespace FlatBufs {


class SchemaInfo {
public:
typedef std::unique_ptr<SchemaInfo> ptr;
virtual MakeFlatBufferFromPVStructure::ptr create_converter() = 0;
};

class SchemaRegistry {
public:
static std::map<std::string, SchemaInfo::ptr> & items();

static void registrate(std::string fbid, SchemaInfo::ptr && si) {
	items()[fbid] = std::move(si);
}

template <typename T>
class Registrar {
public:
Registrar(std::string fbid, SchemaInfo::ptr && si) {
	SchemaRegistry::registrate(fbid, std::move(si));
}
};

};


}
}
