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
    auto &SchemaMap = items();
    if (SchemaMap.find(FlatbufferID) != SchemaMap.end()) {
      auto ErrorString =
          fmt::format("ERROR schema handler for [{:.{}}] exists already",
                      FlatbufferID.data(), FlatbufferID.size());
      throw std::runtime_error(ErrorString);
    }
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
