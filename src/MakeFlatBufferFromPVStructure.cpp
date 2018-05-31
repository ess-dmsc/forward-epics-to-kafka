#include "MakeFlatBufferFromPVStructure.h"

namespace FlatBufs {

MakeFlatBufferFromPVStructure::~MakeFlatBufferFromPVStructure() {}

void MakeFlatBufferFromPVStructure::config(
    std::map<std::string, int64_t> const &config_ints,
    std::map<std::string, std::string> const &config_strings) {}

std::map<std::string, double> MakeFlatBufferFromPVStructure::stats() {
  return {};
}
}
