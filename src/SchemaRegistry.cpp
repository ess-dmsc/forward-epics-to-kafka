#include "SchemaRegistry.h"

namespace FlatBufs {


/// Lets flatbuffer schema plugins register themselves.
///
/// See `src/schemas/f142/f142.cxx` for an example.
///
/// \return A map of schema names and their details
std::map<std::string, SchemaInfo::ptr> &SchemaRegistry::items() {
  static std::map<std::string, SchemaInfo::ptr> _items;
  return _items;
}
}
