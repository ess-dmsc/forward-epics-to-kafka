#include "../Config.h"
#include "../KafkaW/Consumer.h"
#include <CommandHandler.h>
#include <gtest/gtest.h>
#include <helper.h>

namespace KafkaW {
class ConsumerFake : public ConsumerInterface {
public:
  std::unique_ptr<ConsumerMessage> poll() override {
    std::string Data = "1,2,3";
    auto Message =
        make_unique<KafkaW::ConsumerMessage>(Data, KafkaW::PollStatus::Msg);
    return Message;
  };
  void addTopic(const std::string &Topic) override { UNUSED_ARG(Topic); };
};
}

TEST(ListenerTest, successfully_create_listener_and_poll) {
  KafkaW::BrokerSettings bopt;
  auto FakeConsumer = make_unique<KafkaW::ConsumerFake>();

  Forwarder::URI uri;
  Forwarder::Config::Listener listener(uri, std::move(FakeConsumer));
  Forwarder::MainOpt Options;
  Forwarder::Forwarder ForwarderInstance(Options);
  Forwarder::ConfigCB config_cb(ForwarderInstance);
  ASSERT_NO_THROW(listener.poll(config_cb));
}
