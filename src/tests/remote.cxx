#include "Config.h"
#include "Main.h"
#include "MainOpt.h"
#include "helper.h"
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
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
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
  Consumer(KafkaW::BrokerOpt bopt, string topic);
  void run();
  std::atomic<int> do_run{1};
  KafkaW::BrokerOpt bopt;
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

Consumer::Consumer(KafkaW::BrokerOpt bopt, string topic)
    : bopt(bopt), topic(topic) {}

void Consumer::run() {
  KafkaW::Consumer consumer(bopt);
  consumer.on_rebalance_assign =
      [this](rd_kafka_topic_partition_list_t *plist) {
        {
          unique_lock<mutex> lock(mx);
          catched_up = 1;
        }
        cv.notify_all();
      };
  consumer.add_topic(topic);
  while (do_run) {
    auto x = consumer.poll();
    if (auto m = x.is_Msg()) {
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
                     rapidjson::Value &d0) = 0;
  virtual int verify(deque<uptr<Consumer>> &consumers) = 0;
};

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
  rapidjson::Document d0;
  {
    d0.Parse("{\"channel\": \"forwarder_test_nt_array_double\", \"converter\": "
             "{\"schema\":\"f142\", \"topic\":\"tmp-test-f142\"}}");
    ASSERT_FALSE(d0.HasParseError());
  }

  using std::thread;
  KafkaW::BrokerOpt bopt;
  bopt.conf_strings["group.id"] = "forwarder-tests-123213ab";
  bopt.conf_ints["receive.message.max.bytes"] = 25100100;
  bopt.address = Tests::main_opt->brokers_as_comma_list();
  Consumer consumer(bopt, get_string(&d0, "converter.topic"));
  consumer.source_name = get_string(&d0, "channel");
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
  main.mapping_add(d0);

  // Let it do its thing for a few seconds...
  sleep_ms(5000);

  main.forwarding_exit();
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
  auto msg = gulp(cmd_msg_fname);
  rapidjson::Document d0;
  d0.Parse(msg.data(), msg.size());
  ASSERT_FALSE(d0.HasParseError());

  deque<uptr<Consumer>> consumers;
  vector<thread> consumer_threads;

  consumer_verifier.create(consumers, d0);
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
    using namespace rapidjson;
    using namespace KafkaW;
    StringBuffer buf1;
    Writer<StringBuffer> wr(buf1);
    d0.Accept(wr);
    BrokerOpt bopt;
    bopt.address = Tests::main_opt->broker_config.host_port;
    auto pr = std::make_shared<Producer>(bopt);
    ProducerTopic pt(pr, Tests::main_opt->broker_config.topic);
    pt.produce((uchar *)buf1.GetString(), buf1.GetSize());
  }
  LOG(7, "CONFIG has been sent out...");

  // Let it do its thing for a few seconds...
  sleep_ms(20000);

  {
    using namespace rapidjson;
    using namespace KafkaW;
    Document d0;
    d0.Parse("{\"cmd\":\"exit\"}");
    StringBuffer buf1;
    Writer<StringBuffer> wr(buf1);
    d0.Accept(wr);
    BrokerOpt bopt;
    bopt.address = Tests::main_opt->broker_config.host_port;
    auto pr = std::make_shared<Producer>(bopt);
    ProducerTopic pt(pr, Tests::main_opt->broker_config.topic);
    pt.produce((uchar *)buf1.GetString(), buf1.GetSize());
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
  struct A : public ConsumerVerifier {
    int create(deque<uptr<Consumer>> &consumers, rapidjson::Value &d0) {
      int cid = 0;
      auto m = d0.FindMember("streams");
      if (m != d0.MemberEnd()) {
        if (m->value.IsArray()) {
          for (auto &s : m->value.GetArray()) {
            KafkaW::BrokerOpt bopt;
            bopt.conf_strings["group.id"] =
                fmt::format("forwarder-tests-{}--{}", getpid(), cid);
            bopt.conf_ints["receive.message.max.bytes"] = 25100100;
            // bopt.conf_ints["session.timeout.ms"] = 1000;
            bopt.address = Tests::main_opt->brokers_as_comma_list();
            auto channel = get_string(&s, "channel");
            auto mconv = s.FindMember("converter");
            auto push_conv = [&cid, &consumers, &bopt,
                              &channel](rapidjson::Value &s) {
              auto topic = get_string(&s, "topic");
              uri::URI topic_uri(topic);
              topic_uri.host = Tests::main_opt->brokers.at(0).host;
              topic_uri.port = Tests::main_opt->brokers.at(0).port;
              LOG(7, "broker: {}  topic: {}  channel: {}", topic_uri.host_port,
                  topic_uri.topic, channel);
              bopt.address = topic_uri.host_port;
              consumers.push_back(
                  uptr<Consumer>(new Consumer(bopt, topic_uri.topic)));
              auto &c = consumers.back();
              c->source_name = channel;
              ++cid;
            };
            if (mconv != s.MemberEnd()) {
              if (mconv->value.IsObject()) {
                push_conv(mconv->value);
              } else if (mconv->value.IsArray()) {
                for (auto &s : mconv->value.GetArray()) {
                  push_conv(s);
                }
              }
            }
          }
        }
      }
      return 0;
    }
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
    Cons(KafkaW::BrokerOpt bopt, string topic) : Consumer(bopt, topic) {}
    void process_msg_impl(LogData const *fb) {
      if (fb->value_type() == Value::ArrayUInt) {
        auto a = ((ArrayUInt *)fb->value())->value();
        had[0] = a->data()[0];
        had[1] = a->data()[1];
      }
    }
  };
  struct A : public ConsumerVerifier {
    int create(deque<uptr<Consumer>> &consumers, rapidjson::Value &d0) {
      int cid = 0;
      auto m = d0.FindMember("streams");
      if (m != d0.MemberEnd()) {
        if (m->value.IsArray()) {
          for (auto &s : m->value.GetArray()) {
            KafkaW::BrokerOpt bopt;
            bopt.conf_strings["group.id"] =
                fmt::format("forwarder-tests-{}--{}", getpid(), cid);
            bopt.conf_ints["receive.message.max.bytes"] = 25100100;
            // bopt.conf_ints["session.timeout.ms"] = 1000;
            bopt.address = Tests::main_opt->brokers_as_comma_list();
            auto channel = get_string(&s, "channel");
            auto mconv = s.FindMember("converter");
            auto push_conv = [&cid, &consumers, &bopt,
                              &channel](rapidjson::Value &s) {
              auto topic = get_string(&s, "topic");
              uri::URI topic_uri(topic);
              topic_uri.host = Tests::main_opt->brokers.at(0).host;
              topic_uri.port = Tests::main_opt->brokers.at(0).port;
              LOG(7, "broker: {}  topic: {}  channel: {}", topic_uri.host_port,
                  topic_uri.topic, channel);
              bopt.address = topic_uri.host_port;
              consumers.push_back(
                  uptr<Consumer>(new Cons(bopt, topic_uri.topic)));
              auto &c = consumers.back();
              c->source_name = channel;
              ++cid;
            };
            if (mconv != s.MemberEnd()) {
              if (mconv->value.IsObject()) {
                push_conv(mconv->value);
              } else if (mconv->value.IsArray()) {
                for (auto &s : mconv->value.GetArray()) {
                  push_conv(s);
                }
              }
            }
          }
        }
      }
      return 0;
    }
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
  struct A : public ConsumerVerifier {
    int create(deque<uptr<Consumer>> &consumers, rapidjson::Value &d0) {
      int cid = 0;
      auto m = d0.FindMember("streams");
      if (m != d0.MemberEnd()) {
        if (m->value.IsArray()) {
          for (auto &s : m->value.GetArray()) {
            KafkaW::BrokerOpt bopt;
            bopt.conf_strings["group.id"] =
                fmt::format("forwarder-tests-{}--{}", getpid(), cid);
            bopt.conf_ints["receive.message.max.bytes"] = 25100100;
            // bopt.conf_ints["session.timeout.ms"] = 1000;
            bopt.address = Tests::main_opt->brokers_as_comma_list();
            auto channel = get_string(&s, "channel");
            auto mconv = s.FindMember("converter");
            auto push_conv = [&cid, &consumers, &bopt,
                              &channel](rapidjson::Value &s) {
              auto topic = get_string(&s, "topic");
              uri::URI topic_uri(topic);
              topic_uri.host = Tests::main_opt->brokers.at(0).host;
              topic_uri.port = Tests::main_opt->brokers.at(0).port;
              LOG(7, "broker: {}  topic: {}  channel: {}", topic_uri.host_port,
                  topic_uri.topic, channel);
              bopt.address = topic_uri.host_port;
              consumers.push_back(
                  uptr<Consumer>(new Consumer(bopt, topic_uri.topic)));
              auto &c = consumers.back();
              c->source_name = channel;
              ++cid;
            };
            if (mconv != s.MemberEnd()) {
              if (mconv->value.IsObject()) {
                push_conv(mconv->value);
              } else if (mconv->value.IsArray()) {
                for (auto &s : mconv->value.GetArray()) {
                  push_conv(s);
                }
              }
            }
          }
        }
      }
      return 0;
    }
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
