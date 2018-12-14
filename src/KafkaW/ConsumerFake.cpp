#include "ConsumerFake.h"
#include <helper.h>
#include <iostream>

namespace KafkaW {
std::unique_ptr<KafkaW::ConsumerMessage> KafkaW::ConsumerFake::poll() {
  const uint8_t Pointer[2] = {1, 2};
  std::size_t Size = 5;
  auto Message = make_unique<KafkaW::ConsumerMessage>(Pointer, Size,
                                                      KafkaW::PollStatus::Msg);
  return Message;
}

void ConsumerFake::addTopic(const std::string &Topic) {
  std::cout << Topic << std::endl;
}
}