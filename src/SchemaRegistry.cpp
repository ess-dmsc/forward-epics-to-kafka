#include "SchemaRegistry.h"

namespace FlatBufs {

/**
\class SchemaRegistry
\brief Lets flatbuffer schema plugins register themself.

See `src/schemas/f142/f142.cxx` the last 10 lines for an example.
*/

std::map<std::string, SchemaInfo::ptr> &SchemaRegistry::items() {
  static std::map<std::string, SchemaInfo::ptr> _items;
  return _items;
}
}
