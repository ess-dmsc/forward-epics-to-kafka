#pragma once

#include "Msg.h"
#include <memory>

namespace KafkaW {

class PollStatus {
public:
  static PollStatus Ok();
  static PollStatus Err();
  static PollStatus EOP();
  static PollStatus Empty();
  static PollStatus newWithMsg(std::unique_ptr<Msg> Msg);
  PollStatus(PollStatus &&) noexcept;
  PollStatus &operator=(PollStatus &&) noexcept;
  ~PollStatus();
  void reset();
  PollStatus() = default;
  bool isOk() const;
  bool isErr() const;
  bool isEOP() const;
  bool isEmpty() const;
  std::unique_ptr<Msg> isMsg();

private:
  int state = -1;
  void *data = nullptr;
};
}
