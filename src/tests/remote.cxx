#include <gtest/gtest.h>
#include <string>
#include <array>
#include <vector>
#include <memory>
#include <thread>
#include <chrono>
#include <atomic>
#include "../helper.h"
#include "../logger.h"
#include "../MainOpt.h"
#include "tests.h"
#include "../Main.h"
#include "../schemas/f142_logdata_generated.h"

namespace BrightnESS {
namespace ForwardEpicsToKafka {

using std::string;

class Consumer {
public:
Consumer(KafkaW::BrokerOpt bopt, string topic);
void run();
std::atomic<int> do_run {1};
KafkaW::BrokerOpt bopt;
string topic;
int msgs_good = 0;
string source_name;
};

Consumer::Consumer(KafkaW::BrokerOpt bopt, string topic) :
		bopt(bopt),
		topic(topic)
{ }

void Consumer::run() {
	//LOG(3, "TOPIC: {}", topic);
	bopt.conf_ints["session.timeout.ms"] = 6000;
	KafkaW::Consumer consumer(bopt);
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
};

void Remote_T::simple_f142() {
	auto requirements = []{
		LOG(0, "\n\n"
		"This test requires an available Epics PV of normative types array double\n"
		"called 'forwarder_test_nt_array_double' running somewhere on the network.\n"
		"It has to update that PV during the runtime of this test.\n"
		"A NTArrayDouble[40] with e.g. 20 Hz update frequency will do just fine.\n"
		"This test also requires a broker which you can specify using --broker\n"
		"when running the test.\n"
		"The broker has to automatically create the topics that we use.\n"
		);
	};

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


TEST_F(Remote_T, simple_f142) {
	Remote_T::simple_f142();
}

}
}
