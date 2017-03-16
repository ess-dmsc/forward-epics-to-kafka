#include <gtest/gtest.h>
#include <string>
#include <array>
#include <vector>
#include <memory>
#include <thread>
#include <chrono>
#include <atomic>
#include <condition_variable>
#include "../helper.h"
#include "../logger.h"
#include "../MainOpt.h"
#include "tests.h"
#include "../Main.h"
#include "../Config.h"
#include "../schemas/f142_logdata_generated.h"
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

namespace BrightnESS {
namespace ForwardEpicsToKafka {

using std::string;
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
		"This test requires an available Epics PV of normative types array double\n"
		"called 'forwarder_test_nt_array_double' running somewhere on the network.\n"
		"It has to update that PV during the runtime of this test.\n"
		"A NTArrayDouble[40] with e.g. 20 Hz update frequency will do just fine.\n"
		"This test also requires a broker which you can specify using --broker\n"
		"when running the test.\n"
		"The broker has to automatically create the topics that we use.\n"
	);
}

void Remote_T::simple_f142() {
	rapidjson::Document d0;
	{
		using namespace rapidjson;
		d0.SetObject();
		auto & a = d0.GetAllocator();
		d0.AddMember("channel", StringRef("forwarder_test_nt_array_double"), a);
		d0.AddMember("topic", StringRef("tmp-test-f142"), a);
		d0.AddMember("type", StringRef("f142"), a);
	}

	using std::thread;
	KafkaW::BrokerOpt bopt;
	bopt.conf_strings["group.id"] = "forwarder-tests-123213ab";
	bopt.conf_ints["receive.message.max.bytes"] = 25100100;
	bopt.address = Tests::main_opt->brokers_as_comma_list();
	Consumer consumer(bopt, get_string(&d0, "topic"));
	consumer.source_name = get_string(&d0, "channel");
	thread thr_consumer([&consumer]{
		consumer.run();
	});

	//sleep_ms(500);

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
	rapidjson::Document d0;
	{
		using namespace rapidjson;
		using V = rapidjson::Value;
		d0.SetObject();
		auto & a = d0.GetAllocator();
		d0.AddMember("cmd", StringRef("add"), a);
		V v1;
		v1.SetObject();
		v1.AddMember("channel", StringRef("forwarder_test_nt_array_double"), a);
		v1.AddMember("topic", StringRef("tmp-test-f142"), a);
		v1.AddMember("type", StringRef("f142"), a);
		V va;
		va.SetArray();
		va.PushBack(v1, a);
		d0.AddMember("streams", va, a);
	}

	using std::thread;
	KafkaW::BrokerOpt bopt;
	bopt.conf_strings["group.id"] = fmt::format("forwarder-tests-{}", getpid());
	bopt.conf_ints["receive.message.max.bytes"] = 25100100;
	//bopt.conf_ints["session.timeout.ms"] = 1000;
	bopt.address = Tests::main_opt->brokers_as_comma_list();
	Consumer consumer(bopt, get_string(&d0, "streams.0.topic"));
	consumer.source_name = get_string(&d0, "streams.0.channel");
	thread thr_consumer([&consumer]{
		consumer.run();
	});
	{
		unique_lock<mutex> lock(consumer.mx);
		consumer.cv.wait_for(lock, MS(100), [&consumer]{return consumer.catched_up == 1;});
		LOG(3, "CATCHED UP");
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
		using V = rapidjson::Value;
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

	// TODO
	// test here that it has finished...
	for (int i1 = 0; i1 < 50; ++i1) {
		if (main.forwarding_status == ForwardingStatus::STOPPED) break;
		sleep_ms(100);
	}

	ASSERT_EQ(main.forwarding_status.load(), ForwardingStatus::STOPPED);

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


TEST_F(Remote_T, simple_f142) {
	Remote_T::simple_f142();
}

TEST_F(Remote_T, simple_f142_via_config_message) {
	Remote_T::simple_f142_via_config_message();
}

}
}
