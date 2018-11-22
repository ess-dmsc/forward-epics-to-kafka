#include "../Config.h"
#include "../KafkaW/ConsumerFake.h"
#include <CommandHandler.h>
#include <gtest/gtest.h>
#include <helper.h>

TEST(ListenerTest, successfully_create_listener_and_poll) {
  KafkaW::BrokerSettings bopt;
  bopt.ConfigurationStrings["group.id"] =
      fmt::format("forwarder-command-listener--pid{}", getpid());
  auto FakeConsumer = make_unique<KafkaW::ConsumerFake>();

  Forwarder::URI uri;
  Forwarder::Config::Listener listener(bopt, uri, std::move(FakeConsumer));
  Forwarder::Forwarder *F;
  Forwarder::ConfigCB config_cb(*F);
  ASSERT_NO_THROW(listener.poll(config_cb));
}