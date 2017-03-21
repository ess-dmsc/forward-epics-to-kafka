#include <gtest/gtest.h>
#include <string>
#include <array>
#include <vector>
#include <deque>
#include <memory>
#include <thread>
#include <chrono>
#include <atomic>
#include <condition_variable>
#include "tests.h"
#include "../helper.h"
#include "../logger.h"
#include "../MainOpt.h"
#include "../Main.h"
#include "../Config.h"
#include "../schemas/f142_logdata_generated.h"
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

namespace BrightnESS {
namespace ForwardEpicsToKafka {

using std::string;
using std::vector;
using std::deque;
using std::thread;
using std::mutex;
using std::unique_lock;
using std::condition_variable;
using MS = std::chrono::milliseconds;

class Consumer {
public:
Consumer(KafkaW::BrokerOpt bopt, string topic);
void run();
std::atomic<int> do_run {1};
KafkaW::BrokerOpt bopt;
string topic;
int msgs_good = 0;
string source_name;
int catched_up = 0;
mutex mx;
condition_variable cv;
std::function<void()> on_catched_up;
};

Consumer::Consumer(KafkaW::BrokerOpt bopt, string topic) :
		bopt(bopt),
		topic(topic)
{ }

void Consumer::run() {
	KafkaW::Consumer consumer(bopt);
	consumer.on_rebalance_assign = [this](rd_kafka_topic_partition_list_t * plist){
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
						if (string(fb->source_name()->c_str()) == source_name) {
							LOG(7, "Consumer got msg:  size: {}  fbid: {:.4}", m->size(), fbid);
							msgs_good += 1;
						}
					}
				}
			}
		}
	}
}

class Remote_T : public testing::Test {
public:
static void simple_f142();
static void simple_f142_via_config_message();
static void requirements();
};

void Remote_T::requirements() {
	LOG(0, "\n\n"
		"This test requires two available Epics PV:\n"
		"1) Normative Types Array Double 'forwarder_test_nt_array_double'\n"
		"2) Normative Types Array Int32 'forwarder_test_nt_array_int32'\n"
		"They have to update during the runtime of this test.\n"
		"Some 20 Hz update frequency will do just fine.\n"
		"This test also requires a broker which you can specify using --broker\n"
		"when running the test.\n"
		"The broker has to automatically create the topics that we use.\n"
	);
}

void Remote_T::simple_f142() {
	rapidjson::Document d0;
	{
		d0.Parse("{\"channel\": \"forwarder_test_nt_array_double\", \"converter\": {\"schema\":\"f142\", \"topic\":\"tmp-test-f142\"}}");
		ASSERT_FALSE(d0.HasParseError());
	}

	using std::thread;
	KafkaW::BrokerOpt bopt;
	bopt.conf_strings["group.id"] = "forwarder-tests-123213ab";
	bopt.conf_ints["receive.message.max.bytes"] = 25100100;
	bopt.address = Tests::main_opt->brokers_as_comma_list();
	Consumer consumer(bopt, get_string(&d0, "converter.topic"));
	consumer.source_name = get_string(&d0, "channel");
	thread thr_consumer([&consumer]{
		consumer.run();
	});

	BrightnESS::ForwardEpicsToKafka::Main main(*Tests::main_opt);
	thread thr_forwarder([&main]{
		try {
			main.forward_epics_to_kafka();
		}
		catch (std::runtime_error & e) {
			LOG(0, "CATCH runtime error in main watchdog thread: {}", e.what());
		}
		catch (std::exception & e) {
			LOG(0, "CATCH EXCEPTION in main watchdog thread");
		}
	});

	//sleep_ms(500);
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


void Remote_T::simple_f142_via_config_message() {
	LOG(3, "This test should complete within about 20 seconds.");
	// Make a sample configuration with two streams
	auto msg = gulp("tests/msg-add-01.json");
	rapidjson::Document d0;
	d0.Parse(msg.data(), msg.size());
	ASSERT_FALSE(d0.HasParseError());

	deque<Consumer> consumers;
	vector<thread> consumer_threads;

	for (int i1 = 0; i1 < 2; ++i1) {
		KafkaW::BrokerOpt bopt;
		bopt.conf_strings["group.id"] = fmt::format("forwarder-tests-{}--{}", getpid(), i1);
		bopt.conf_ints["receive.message.max.bytes"] = 25100100;
		//bopt.conf_ints["session.timeout.ms"] = 1000;
		bopt.address = Tests::main_opt->brokers_as_comma_list();
		consumers.emplace_back(bopt, get_string(&d0, fmt::format("streams.{}.converter.topic", i1)));
		auto & c = consumers.back();
		c.source_name = get_string(&d0, fmt::format("streams.{}.channel", i1));
		consumer_threads.emplace_back([&c]{
			c.run();
		});
	}
	{
		int i1 = 0;
		for (auto & c : consumers) {
			unique_lock<mutex> lock(c.mx);
			c.cv.wait_for(lock, MS(100), [&c]{return c.catched_up == 1;});
			LOG(7, "Consumer {} catched up", i1);
			++i1;
		}
	}

	BrightnESS::ForwardEpicsToKafka::Main main(*Tests::main_opt);
	thread thr_forwarder([&main]{
		try {
			main.forward_epics_to_kafka();
		}
		catch (std::runtime_error & e) {
			LOG(0, "CATCH runtime error in main watchdog thread: {}", e.what());
		}
		catch (std::exception & e) {
			LOG(0, "CATCH EXCEPTION in main watchdog thread");
		}
	});
	if (!main.config_listener) {
		LOG(0, "\n\nNOTE:  Please use --broker-config <//host[:port]/topic> of your configuration topic.\n");
	}
	ASSERT_NE(main.config_listener.get(), nullptr);
	main.config_listener->wait_for_connected(MS(1000));
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
		Producer pr(bopt);
		ProducerTopic pt(pr, Tests::main_opt->broker_config.topic);
		pt.produce((uchar*)buf1.GetString(), buf1.GetSize());
	}
	LOG(7, "CONFIG has been sent out...");

	// Let it do its thing for a few seconds...
	sleep_ms(10000);

	{
		using namespace rapidjson;
		using namespace KafkaW;
		Document d0;
		d0.SetObject();
		auto & a = d0.GetAllocator();
		d0.AddMember("cmd", StringRef("exit"), a);
		StringBuffer buf1;
		Writer<StringBuffer> wr(buf1);
		d0.Accept(wr);
		BrokerOpt bopt;
		bopt.address = Tests::main_opt->broker_config.host_port;
		Producer pr(bopt);
		ProducerTopic pt(pr, Tests::main_opt->broker_config.topic);
		pt.produce((uchar*)buf1.GetString(), buf1.GetSize());
	}

	// Give it a chance to exit by itself...
	for (int i1 = 0; i1 < 50; ++i1) {
		if (main.forwarding_status == ForwardingStatus::STOPPED) break;
		sleep_ms(100);
	}
	ASSERT_EQ(main.forwarding_status.load(), ForwardingStatus::STOPPED);

	if (thr_forwarder.joinable()) {
		thr_forwarder.join();
	}

	for (auto & c : consumers) {
		c.do_run = 0;
	}
	for (auto & t : consumer_threads) {
		if (t.joinable()) {
			t.join();
		}
	}

	for (auto & c : consumers) {
		LOG(6, "Consumer received {} messages", c.msgs_good);
		// Arbitrarily require at least 3 received messages
		if (c.msgs_good < 3) {
			requirements();
		}
		ASSERT_GT(c.msgs_good, 0);
	}

	LOG(4, "All done, test exit");
}


TEST_F(Remote_T, simple_f142) {
	Remote_T::simple_f142();
}

TEST_F(Remote_T, simple_f142_via_config_message) {
	Remote_T::simple_f142_via_config_message();
}

}
}
