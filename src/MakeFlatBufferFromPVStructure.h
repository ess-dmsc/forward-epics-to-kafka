#pragma once
#include <memory>
#include <map>
#include <string>
#include "fbschemas.h"

namespace BrightnESS {
namespace FlatBufs {

struct EpicsPVUpdate;

/// Interface for flat buffer creators for the different schemas
class MakeFlatBufferFromPVStructure {
public:
  typedef std::unique_ptr<MakeFlatBufferFromPVStructure> ptr;
  typedef std::shared_ptr<MakeFlatBufferFromPVStructure> sptr;
  virtual BrightnESS::FlatBufs::FB_uptr convert(EpicsPVUpdate const &up) = 0;
  virtual void config(std::map<std::string, int64_t> const &config_ints,
                      std::map<std::string, std::string> const &config_strings);
};
}
}
