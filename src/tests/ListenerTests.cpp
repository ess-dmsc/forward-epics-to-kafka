// SPDX-License-Identifier: BSD-2-Clause
//
// This code has been produced by the European Spallation Source
// and its partner institutes under the BSD 2 Clause License.
//
// See LICENSE.md at the top level for license information.
//
// Screaming Udder!                              https://esss.se

#include "CommandHandler.h"
#include "Config.h"
#include "Forwarder.h"
#include <KafkaW/Consumer.h>
#include <gtest/gtest.h>

namespace KafkaW {
class ConsumerFake : public ConsumerInterface {
public:
  std::unique_ptr<ConsumerMessage> poll() override {
    std::string Data = "1,2,3";
    auto Message = std::make_unique<KafkaW::ConsumerMessage>(
        Data, KafkaW::PollStatus::Message);
    return Message;
  };
  void addTopic(const std::string &Topic) override { UNUSED_ARG(Topic); };
};
} // namespace KafkaW

TEST(ListenerTest, successfully_create_listener_and_poll) {
  KafkaW::BrokerSettings bopt;
  auto FakeConsumer = std::make_unique<KafkaW::ConsumerFake>();

  Forwarder::URI uri;
  Forwarder::Config::Listener listener(uri, std::move(FakeConsumer));
  Forwarder::MainOpt Options;
  Forwarder::Forwarder ForwarderInstance(Options);
  Forwarder::ConfigCallback config_cb(ForwarderInstance);
  ASSERT_NO_THROW(listener.poll(config_cb));
}
