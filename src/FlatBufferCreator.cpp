#include "FlatBufferCreator.h"

namespace FlatBufs {

void FlatBufferCreator::config(
    std::map<std::string, std::string> const &KafkaConfiguration) {
  UNUSED_ARG(KafkaConfiguration);
}

std::map<std::string, double> FlatBufferCreator::getStats() { return {}; }
} // namespace FlatBufs
