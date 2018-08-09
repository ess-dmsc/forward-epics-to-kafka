#include "FlatBufferCreator.h"

namespace FlatBufs {

void FlatBufferCreator::config(
    std::map<std::string, int64_t> const &config_ints,
    std::map<std::string, std::string> const &config_strings) {
  UNUSED_ARG(config_ints);
  UNUSED_ARG(config_strings);
}

std::map<std::string, double> FlatBufferCreator::getStats() { return {}; }
} // namespace FlatBufs
