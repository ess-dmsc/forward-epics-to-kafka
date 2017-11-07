#include "gmock/gmock.h"
#include "../KafkaW.h"
#include "../Config.h"
#include "../uri.h"
#include "../helper.h"
#include <mutex>
#include <condition_variable>
using namespace testing;

class cb_fake : public BrightnESS::ForwardEpicsToKafka::Config::Callback {
public:
  void operator()(std::string const &msg) override {m_message = msg;};
  std::string get_message(){return m_message;};
private:
  std::string m_message;
};

class FakeMsg : public KafkaW::AbstractMsg {
  using uchar = unsigned char;
  unsigned char some_data[4] = {'t', 'e', 's', 't'};
public:
  FakeMsg(std::string msg) : KafkaW::AbstractMsg() {
    kmsg = static_cast<void*>(&msg);
  }

  FakeMsg() = default;

  uchar *data() override {
    return some_data;
  }

  uint32_t size() override {
    uint32_t s = 4;
    return s;
  }

  char const *topic_name() override {return "test_topic_name";};
  int32_t offset() override {return 1;};
  int32_t partition() override {return 1;};

};


class FakePollStatus : public KafkaW::PollStatus {
public:
  std::unique_ptr<KafkaW::AbstractMsg> is_Msg() override {
    FakeMsg mesg("test");
    return make_unique<FakeMsg>(mesg);
  };
//  static FakePollStatus make_Msg(std::unique_ptr<KafkaW::Msg> x)=default;

private:
  int state = -1;
  void *data = nullptr;
};

class MockConsumer : public KafkaW::BaseConsumer {
public:
  MockConsumer()=default;
  virtual ~MockConsumer(){};
  MockConsumer(const MockConsumer&){};
  MOCK_METHOD0(init, void());
  MOCK_METHOD1(add_topic, void(std::string topic));
  MockConsumer& operator=(const MockConsumer)=delete;

  KafkaW::PollStatus poll() override {
    std::unique_ptr<FakeMsg> m2(new FakeMsg);
    m2->kmsg = (void *) "hello";
    return FakePollStatus::make_Msg(std::move(m2));
//    auto msg = make_unique<KafkaW::Msg>();
//    return status.make_Msg(msg);
  }
};

//
//TEST(kafkaw_consumer_tests, test_something) {
//  MockConsumer mc;
//  EXPECT_CALL(mc, poll()).Times(AtLeast(1));
//  mc.poll();
//}
//
//TEST(kafkaw_consumer_tests, test_something_else) {
//  MockConsumer mc;
//  EXPECT_CALL(mc, poll()).Times(AtLeast(1));
//  mc.init();
//}

//TEST(kafkaw_consumer_tests, test_config_polling) {
//
////  EXPECT_CALL(mc, poll()).Times(AtLeast(1));
////  std::unique_ptr<KafkaW::BaseConsumer> ptr = make_unique<MockConsumer>(mc);
//  auto ptr = std::make_shared<MockConsumer>();
//  EXPECT_CALL(*ptr, poll());
//  BrightnESS::ForwardEpicsToKafka::Config::Listener listener1(ptr);
//  auto cb = cb_fake();
//  listener1.poll(cb);
//  std::cout << cb.get_message() << std::endl;
//}

TEST(kafkaw_consumer_tests, test_config_polling_returns_msg) {
  std::string expected = "test";
  auto consumer_ptr = make_unique<MockConsumer>();
  BrightnESS::ForwardEpicsToKafka::Config::Listener listener1(std::move(consumer_ptr));
  auto cb = cb_fake();
  listener1.poll(cb);
  ASSERT_EQ(std::string(cb.get_message()), expected);
}