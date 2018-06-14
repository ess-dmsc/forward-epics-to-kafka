#pragma once

#include <cstdint>
#include <cstdlib>

namespace KafkaW {
// Want to expose this typedef also for users of this namespace
using uchar = unsigned char;

class Msg {
public:
  ~Msg();
  uchar *data() const;
  size_t size() const;
  void *MsgPtr;
};
}
