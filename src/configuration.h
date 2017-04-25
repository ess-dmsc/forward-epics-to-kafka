#pragma once

#include <deque>
#include <string>

namespace BrightnESS {
namespace ForwardEpicsToKafka {

class Configuration {
public:
  void read_json(std::string fname);

private:
  // std::deque<>
};
}
}
