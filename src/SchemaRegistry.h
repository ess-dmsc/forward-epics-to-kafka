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
  static std::map<std::string, SchemaInfo::ptr> &items();

  static void registerSchema(std::string const &FlatbufferID,
                             SchemaInfo::ptr &&SchemaInfoPtr) {
    auto Logger = getLogger();
    auto &SchemaMap = items();
    if (SchemaMap.find(FlatbufferID) != SchemaMap.end()) {
      Logger->warn(
          "schema handler for [{:.{}}] exists already\",\n"
          "                      FlatbufferID.data(), FlatbufferID.size()");
    }
    Logger->trace("Registered schema {}", FlatbufferID);
    SchemaMap[FlatbufferID] = std::move(SchemaInfoPtr);
  }

  template <typename T> class Registrar {
  public:
    Registrar(std::string FlatbufferID, SchemaInfo::ptr &&SchemaInfoPtr) {
      SchemaRegistry::registerSchema(std::move(FlatbufferID),
                                     std::move(SchemaInfoPtr));
    }
  };
};
} // namespace FlatBufs
