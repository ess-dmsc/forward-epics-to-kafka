#include "PollStatus.h"
#include <algorithm>

namespace KafkaW {

PollStatus::~PollStatus() { reset(); }

PollStatus PollStatus::Ok() {
  PollStatus ret;
  ret.state = 0;
  return ret;
}

PollStatus PollStatus::Err() {
  PollStatus ret;
  ret.state = -1;
  return ret;
}

PollStatus PollStatus::EOP() {
  PollStatus ret;
  ret.state = -2;
  return ret;
}

PollStatus PollStatus::Empty() {
  PollStatus ret;
  ret.state = -3;
  return ret;
}

PollStatus PollStatus::newWithMsg(std::unique_ptr<Msg> Msg) {
  PollStatus ret;
  ret.state = 1;
  ret.data = Msg.release();
  return ret;
}

PollStatus::PollStatus(PollStatus &&x) noexcept
    : state(x.state), data(x.data) {}

PollStatus &PollStatus::operator=(PollStatus &&x) noexcept {
  reset();
  std::swap(state, x.state);
  std::swap(data, x.data);
  return *this;
}

void PollStatus::reset() {
  if (state == 1) {
    if (auto x = reinterpret_cast<Msg *>(data)) {
      delete x;
    }
  }
  state = -1;
  data = nullptr;
}

bool PollStatus::isOk() const { return state == 0; }

bool PollStatus::isErr() const { return state == -1; }

bool PollStatus::isEOP() const { return state == -2; }

bool PollStatus::isEmpty() const { return state == -3; }

std::unique_ptr<Msg> PollStatus::isMsg() {
  if (state == 1) {
    std::unique_ptr<Msg> ret(reinterpret_cast<Msg *>(data));
    data = nullptr;
    return ret;
  }
  return nullptr;
}
}
