#pragma once
#include "MakeFlatBufferFromPVStructure.h"
#include "logger.h"
#include <map>
#include <memory>
#include <string>

namespace FlatBufs {

class SchemaInfo {
public:
  typedef std::unique_ptr<SchemaInfo> ptr;
  virtual MakeFlatBufferFromPVStructure::ptr create_converter() = 0;
};

class SchemaRegistry {
public:
  static std::map<std::string, SchemaInfo::ptr> &items();

  static void registrate(std::string fbid, SchemaInfo::ptr &&si) {
    auto &m = items();
    if (m.find(fbid) != m.end()) {
      auto s = fmt::format("ERROR schema handler for [{:.{}}] exists already",
                           fbid.data(), fbid.size());
      throw std::runtime_error(s);
    }
    m[fbid] = std::move(si);
  }

  template <typename T> class Registrar {
  public:
    Registrar(std::string fbid, SchemaInfo::ptr &&si) {
      SchemaRegistry::registrate(fbid, std::move(si));
    }
  };
};
}
