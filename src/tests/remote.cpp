#include "Config.h"
#include "Main.h"
#include "MainOpt.h"
#include "helper.h"
#include "json.h"
#include "logger.h"
#include "schemas/f142_logdata_generated.h"
#include "tests.h"
#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <thread>
#include <vector>

namespace BrightnESS {
namespace ForwardEpicsToKafka {
namespace tests {

using std::string;
using std::vector;
using std::deque;
using std::thread;
using std::mutex;
using std::unique_lock;
using std::condition_variable;
using MS = std::chrono::milliseconds;
template <typename T> using uptr = std::unique_ptr<T>;

class Consumer {
public:
  Consumer(KafkaW::BrokerSettings BrokerSettings, string topic);
  void run();
  std::atomic<int> do_run{1};
  KafkaW::BrokerSettings BrokerSettings;
  string topic;
  int msgs_good = 0;
  string source_name;
  int catched_up = 0;
  mutex mx;
  condition_variable cv;
  std::function<void()> on_catched_up;
  void process_msg(LogData const *fb);
  virtual void process_msg_impl(LogData const *fb);
};

Consumer::Consumer(KafkaW::BrokerSettings BrokerSettings, string topic)
    : BrokerSettings(BrokerSettings), topic(topic) {}

void Consumer::run() {
  KafkaW::Consumer consumer(BrokerSettings);
  consumer.on_rebalance_assign =
      [this](rd_kafka_topic_partition_list_t *plist) {
        {
          unique_lock<mutex> lock(mx);
          catched_up = 1;
        }
        cv.notify_all();
      };
  consumer.addTopic(topic);
  while (do_run) {
    auto x = consumer.poll();
    if (auto m = x.isMsg()) {
      if (m->size() >= 8) {
        auto fbid = m->data() + 4;
        if (memcmp(fbid, "f142", 4) == 0) {
          flatbuffers::Verifier veri(m->data(), m->size());
          if (VerifyLogDataBuffer(veri)) {
            auto fb = GetLogData(m->data());
            auto sn = fb->source_name();
            if (sn) {
              if (string(sn->c_str()) == source_name) {
                process_msg(fb);
              }
            }
          }
        }
      }
    }
  }
}

void Consumer::process_msg(LogData const *fb) {
  process_msg_impl(fb);
  msgs_good += 1;
  if (false) {
    // LOG(9, "Consumer got msg:  size: {}  fbid: {:.4}", m->size(), fbid);
    if (fb->value_type() == Value::ArrayDouble) {
      auto a1 = (ArrayDouble *)fb->value();
      for (uint32_t i1 = 0; i1 < a1->value()->Length(); ++i1) {
        LOG(7, "{}", a1->value()->Get(i1));
      }
    }
  }
}

void Consumer::process_msg_impl(LogData const *fb) {}

class ConsumerVerifier {
public:
  virtual int create(deque<uptr<Consumer>> &consumers,
                     nlohmann::json const &JSON) = 0;
  virtual int verify(deque<uptr<Consumer>> &consumers) = 0;
};

class ConsumerVerifierDefaultCreate : public ConsumerVerifier {
  int create(deque<uptr<Consumer>> &consumers,
             nlohmann::json const &JSON) override;
};

int ConsumerVerifierDefaultCreate::create(deque<uptr<Consumer>> &consumers,
                                          nlohmann::json const &JSON) {
  using nlohmann::json;
  int cid = 0;
  if (auto x = find<json>("streams", JSON)) {
    auto const &Streams = x.inner();
    if (Streams.is_array()) {
      for (auto const &Stream : Streams) {
        KafkaW::BrokerSettings BrokerSettings;
        BrokerSettings.ConfigurationStrings["group.id"] =
            fmt::format("forwarder-tests-{}--{}", getpid(), cid);
        BrokerSettings.ConfigurationIntegers["receive.message.max.bytes"] =
            25100100;
        // BrokerSettings.ConfigurationIntegers["session.timeout.ms"] =
        // 1000;
        BrokerSettings.Address = Tests::main_opt->brokers_as_comma_list();
        auto Channel = find<string>("channel", Stream).inner();
        auto push_conv = [&cid, &consumers, &BrokerSettings,
                          &Channel](json const &Converter) {
          auto Topic = find<string>("topic", Converter).inner();
          uri::URI TopicURI(Topic);
          TopicURI.host = Tests::main_opt->brokers.at(0).host;
          TopicURI.port = Tests::main_opt->brokers.at(0).port;
          LOG(7, "broker: {}  topic: {}  channel: {}", TopicURI.host_port,
              TopicURI.topic, Channel);
          BrokerSettings.Address = TopicURI.host_port;
          consumers.push_back(
              uptr<Consumer>(new Consumer(BrokerSettings, TopicURI.topic)));
          auto &c = consumers.back();
          c->source_name = Channel;
          ++cid;
        };
        if (auto x = find<json>("converter", Stream)) {
          auto const &Converters = x.inner();
          if (Converters.is_object()) {
            push_conv(Converters);
          } else if (Converters.is_array()) {
            for (auto const &Converter : Converters) {
              push_conv(Converter);
            }
          }
        }
      }
    }
  }
  return 0;
}

class Remote_T : public testing::Test {
public:
  static void simple_f142();
  static void
  simple_f142_via_config_message(string cmd_msg_fname,
                                 ConsumerVerifier &consumer_verifier);
  static void requirements();
};

void Remote_T::requirements() {
  LOG(0,
      "\n\n"
      "This test requires two available Epics PV:\n"
      "1) Normative Types Array Double 'forwarder_test_nt_array_double'\n"
      "2) Normative Types Array Int32 'forwarder_test_nt_array_int32'\n"
      "They have to update during the runtime of this test.\n"
      "Some 20 Hz update frequency will do just fine.\n"
      "This test also requires a broker which you can specify using --broker\n"
      "when running the test.\n"
      "The broker has to automatically create the topics that we use.\n");
}

void Remote_T::simple_f142() {
  using nlohmann::json;
  using std::thread;
  using std::string;
  auto Doc = json::parse("{\"channel\": \"forwarder_test_nt_array_double\", "
                         "\"converter\": {\"schema\":\"f142\", "
                         "\"topic\":\"tmp-test-f142\"}}");
  KafkaW::BrokerSettings BrokerSettings;
  BrokerSettings.ConfigurationStrings["group.id"] = "forwarder-tests-123213ab";
  BrokerSettings.ConfigurationIntegers["receive.message.max.bytes"] = 25100100;
  BrokerSettings.Address = Tests::main_opt->brokers_as_comma_list();
  Consumer consumer(BrokerSettings, Doc["converter"]["topic"].get<string>());
  consumer.source_name = Doc["channel"].get<string>();
  thread thr_consumer([&consumer] { consumer.run(); });

  BrightnESS::ForwardEpicsToKafka::Main main(*Tests::main_opt);
  thread thr_forwarder([&main] {
    try {
      main.forward_epics_to_kafka();
    } catch (std::runtime_error &e) {
      LOG(0, "CATCH runtime error in main watchdog thread: {}", e.what());
    } catch (std::exception &e) {
      LOG(0, "CATCH EXCEPTION in main watchdog thread");
    }
  });

  // sleep_ms(500);
  main.mappingAdd(Doc);

  // Let it do its thing for a few seconds...
  sleep_ms(5000);

  main.stopForwarding();
  if (thr_forwarder.joinable()) {
    thr_forwarder.join();
  }

  sleep_ms(500);
  consumer.do_run = 0;
  if (thr_consumer.joinable()) {
    thr_consumer.join();
  }

  if (consumer.msgs_good <= 0) {
    requirements();
  }
  ASSERT_GT(consumer.msgs_good, 0);

  LOG(4, "All done, test exit");
}

void Remote_T::simple_f142_via_config_message(
    string cmd_msg_fname, ConsumerVerifier &consumer_verifier) {
  LOG(3, "This test should complete within about 30 seconds.");
  // Make a sample configuration with two streams
  auto msg = readFile(cmd_msg_fname);
  using nlohmann::json;
  auto Doc = json::parse(msg);

  deque<uptr<Consumer>> consumers;
  vector<thread> consumer_threads;

  consumer_verifier.create(consumers, Doc);
  {
    for (auto &c : consumers) {
      consumer_threads.emplace_back([&c] { c->run(); });
    }
    int i1 = 0;
    for (auto &c_ : consumers) {
      auto &c = *c_;
      unique_lock<mutex> lock(c.mx);
      c.cv.wait_for(lock, MS(100), [&c] { return c.catched_up == 1; });
      LOG(7, "Consumer {} catched up", i1);
      ++i1;
    }
  }

  std::unique_ptr<BrightnESS::ForwardEpicsToKafka::Main> main(
      new BrightnESS::ForwardEpicsToKafka::Main(*Tests::main_opt));
  thread thr_forwarder([&main] {
    try {
      main->forward_epics_to_kafka();
    } catch (std::runtime_error &e) {
      LOG(0, "CATCH runtime error in main watchdog thread: {}", e.what());
    } catch (std::exception &e) {
      LOG(0, "CATCH EXCEPTION in main watchdog thread");
    }
    LOG(7, "thr_forwarder done");
  });
  if (!main->config_listener) {
    LOG(0, "\n\nNOTE:  Please use --broker-config <//host[:port]/topic> of "
           "your configuration topic.\n");
  }
  ASSERT_NE(main->config_listener.get(), nullptr);
  main->config_listener->wait_for_connected(MS(1000));
  LOG(7, "OK config listener connected");
  sleep_ms(1000);

  {
    KafkaW::BrokerSettings BrokerSettings;
    BrokerSettings.Address = Tests::main_opt->BrokerConfig.host_port;
    auto pr = std::make_shared<KafkaW::Producer>(BrokerSettings);
    KafkaW::ProducerTopic pt(pr, Tests::main_opt->BrokerConfig.topic);
    pt.produce((KafkaW::uchar *)msg.data(), msg.size());
  }
  LOG(7, "CONFIG has been sent out...");

  // Let it do its thing for a few seconds...
  sleep_ms(20000);

  {
    auto Msg = string(R"""({"cmd": "exit"})""");
    KafkaW::BrokerSettings BrokerSettings;
    BrokerSettings.Address = Tests::main_opt->BrokerConfig.host_port;
    auto pr = std::make_shared<KafkaW::Producer>(BrokerSettings);
    KafkaW::ProducerTopic pt(pr, Tests::main_opt->BrokerConfig.topic);
    pt.produce((KafkaW::uchar *)Msg.data(), Msg.size());
  }

  // Give it a chance to exit by itself...
  {
    auto t1 = std::chrono::system_clock::now();
    for (int i1 = 0; i1 < 200; ++i1) {
      if (main->forwarding_status == ForwardingStatus::STOPPED)
        break;
      sleep_ms(100);
    }
    auto t2 = std::chrono::system_clock::now();
    LOG(7, "Took Main {} ms to stop",
        std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count());
  }
  ASSERT_EQ(main->forwarding_status.load(), ForwardingStatus::STOPPED);

  if (thr_forwarder.joinable()) {
    thr_forwarder.join();
  }
  main.reset();
  sleep_ms(2000);
  LOG(7, "Main should be dtored by now");

  for (auto &c : consumers) {
    c->do_run = 0;
  }
  for (auto &t : consumer_threads) {
    if (t.joinable()) {
      t.join();
    }
  }

  ASSERT_EQ(consumer_verifier.verify(consumers), 0);

  LOG(4, "All done, test exit");
}

TEST_F(Remote_T, simple_f142) { Remote_T::simple_f142(); }

TEST_F(Remote_T, simple_f142_via_config_message) {
  struct A : public ConsumerVerifierDefaultCreate {
    int verify(deque<uptr<Consumer>> &consumers) {
      int ret = 0;
      for (auto &c : consumers) {
        LOG(6, "Consumer received {} messages", c->msgs_good);
        if (c->msgs_good < 5) {
          ret = 1;
          requirements();
        }
      }
      return ret;
    }
  };
  A cv;
  Remote_T::simple_f142_via_config_message("tests/msg-add-03.json", cv);
}

TEST_F(Remote_T, named_converter) {
  struct Cons : public Consumer {
    std::array<uint32_t, 2> had{{0, 0}};
    Cons(KafkaW::BrokerSettings BrokerSettings, string topic)
        : Consumer(BrokerSettings, topic) {}
    void process_msg_impl(LogData const *fb) {
      if (fb->value_type() == Value::ArrayUInt) {
        auto a = ((ArrayUInt *)fb->value())->value();
        had[0] = a->data()[0];
        had[1] = a->data()[1];
      }
    }
  };
  struct A : public ConsumerVerifierDefaultCreate {
    int verify(deque<uptr<Consumer>> &consumers) {
      int ret = 0;
      for (auto &c_ : consumers) {
        auto &c = *(Cons *)c_.get();
        LOG(6, "Consumer received {} messages", c.msgs_good);
        if (c.msgs_good < 5) {
          ret = 1;
          requirements();
        }
        if (c.had[0] < 8 || c.had[1] < 8) {
          LOG(3, "The single converter instance did not receive messages from "
                 "both channels");
          ret = 1;
        }
      }
      return ret;
    }
  };
  A cv;
  Remote_T::simple_f142_via_config_message("tests/msg-add-named-converter.json",
                                           cv);
}

TEST_F(Remote_T, different_brokers) {
  // Disabled because we have to use different brokers
  return;
  struct A : public ConsumerVerifierDefaultCreate {
    int verify(deque<uptr<Consumer>> &consumers) {
      int ret = 0;
      for (auto &c_ : consumers) {
        auto &c = *c_.get();
        LOG(6, "Consumer received {} messages", c.msgs_good);
        if (c.msgs_good < 5) {
          ret = 1;
          requirements();
        }
      }
      return ret;
    }
  };
  A cv;
  Remote_T::simple_f142_via_config_message(
      "tests/msg-add-different-brokers.json", cv);
}
}
}
}
