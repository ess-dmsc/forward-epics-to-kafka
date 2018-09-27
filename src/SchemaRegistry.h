#include <utility>

#pragma once
#include "FlatBufferCreator.h"
#include "logger.h"
#include <map>
#include <memory>
#include <string>

namespace FlatBufs {

class SchemaInfo {
public:
  typedef std::unique_ptr<SchemaInfo> ptr;
  virtual std::unique_ptr<FlatBufferCreator> createConverter() = 0;
  virtual ~SchemaInfo() = default;
};

class SchemaRegistry {
public:
  static std::map<std::string, SchemaInfo::ptr> &Items();

  static void registrate(std::string FlatbufferID, SchemaInfo::ptr &&SchemaInfoPtr) {
    auto &SchemaMap = Items();
    if (SchemaMap.find(FlatbufferID) != SchemaMap.end()) {
      auto ErrorString = fmt::format("ERROR schema handler for [{:.{}}] exists already",
                           FlatbufferID.data(), FlatbufferID.size());
      throw std::runtime_error(ErrorString);
    }
    SchemaMap[FlatbufferID] = std::move(SchemaInfoPtr);
  }

  template <typename T> class Registrar {
  public:
    Registrar(std::string FlatbufferID, SchemaInfo::ptr &&SchemaInfoPtr) {
      SchemaRegistry::registrate(std::move(FlatbufferID), std::move(SchemaInfoPtr));
    }
  };
};
}
