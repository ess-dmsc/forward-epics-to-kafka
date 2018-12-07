#pragma once

#include "Consumer.h"
#include "Message.h"

namespace KafkaW {
class ConsumerFake : public ConsumerInterface {
public:
  std::unique_ptr<Message> poll() override;
  virtual void addTopic(std::string Topic) override;
};
}