#include "../Config.h"
#include "../KafkaW.h"
#include "../helper.h"
#include "../uri.h"
#include "gmock/gmock.h"
#include <condition_variable>
#include <mutex>
using namespace testing;

class cb_fake : public BrightnESS::ForwardEpicsToKafka::Config::Callback {
public:
  void operator()(std::string const &msg) override { m_message = msg; };
  std::string get_message() { return m_message; };

private:
  std::string m_message;
};

class FakeMsg : public KafkaW::AbstractMsg {
  using uchar = unsigned char;
  unsigned char some_data[4] = {'t', 'e', 's', 't'};

public:
  FakeMsg(std::string message) : KafkaW::AbstractMsg() {
    kmsg = static_cast<void *>(&message);
  }

  FakeMsg() = default;

  uchar *data() override { return some_data; }

  uint32_t size() override {
    uint32_t s = 4;
    return s;
  }

  char const *topic_name() override { return "test_topic_name"; };
  int32_t offset() override { return 1; };
  int32_t partition() override { return 1; };
};

class FakePollStatus : public KafkaW::PollStatus {
public:
  std::unique_ptr<KafkaW::AbstractMsg> is_Msg() override {
    FakeMsg message("test");
    return make_unique<FakeMsg>(message);
  };

private:
  int state = -1;
  void *data = nullptr;
};

class MockConsumer : public KafkaW::BaseConsumer {
public:
  MockConsumer() = default;
  virtual ~MockConsumer(){};
  MockConsumer(const MockConsumer &){};
  MOCK_METHOD0(init, void());
  MOCK_METHOD1(add_topic, void(std::string topic));
  MockConsumer &operator=(const MockConsumer) = delete;

  KafkaW::PollStatus poll() override {
    std::unique_ptr<FakeMsg> fake_message(new FakeMsg);
    fake_message->kmsg = (void *)"hello";
    return FakePollStatus::make_Msg(std::move(fake_message));
  }
};

TEST(kafkaw_consumer_tests, test_config_polling_returns_msg) {
  std::string expected = "test";
  auto consumer_ptr = make_unique<MockConsumer>();
  BrightnESS::ForwardEpicsToKafka::Config::Listener listener(
      std::move(consumer_ptr));
  auto cb = cb_fake();
  listener.poll(cb);
  ASSERT_EQ(std::string(cb.get_message()), expected);
}
