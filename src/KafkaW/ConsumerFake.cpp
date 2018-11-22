#include "ConsumerFake.h"
#include <helper.h>
#include <iostream>
namespace KafkaW {
std::unique_ptr<KafkaW::Message> KafkaW::ConsumerFake::poll() {
  const uint8_t *tu;
  std::size_t n = 5;
  auto Message = make_unique<KafkaW::Message>(tu, n, KafkaW::PollStatus::Msg);
  return Message;
}

void ConsumerFake::addTopic(std::string Topic) {
  std::cout << Topic << std::endl;
}
}