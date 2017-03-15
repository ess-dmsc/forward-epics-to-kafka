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
		//LOG(3, "!~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~CONSUMER POLL");
		auto x = consumer.poll();
		if (x.is_Msg()) {
			LOG(3, "--  got one  --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------");
		}
	}
}

class Remote_T : public testing::Test {
public:
static void simple_f142();
};

void Remote_T::simple_f142() {
	// This test requires an available Epics PV of normative types array double
	// called 'forwarder_test_nt_array_double' running somewhere on the network.
	// This also requires a broker which automatically creates the topics
	// that we use.

	// Do not test config file parsing here.
	// Take the parsed MainOpt
	// Use that factored method to parse my stream config
	rapidjson::Document d0;
	{
		using namespace rapidjson;
		d0.SetObject();
		auto & a = d0.GetAllocator();
		d0.AddMember("channel", StringRef("forwarder_test_nt_array_double"), a);
		d0.AddMember("topic", StringRef("tmp-test-f142"), a);
		d0.AddMember("type", StringRef("f142"), a);
	}

	// - Let the Consumer in this file count the packets
	//   Private variable accessible to test class?
	// Let Main run for some time.
	// Start a Kafka verifier in another thread
	//   Seek to the end
	// Fetch latest offset before forwarding to make sure that we catched up
	// Poll the messages from the 'verifier'
	//   Count only those with the correct FBID
	using std::thread;
	KafkaW::BrokerOpt bopt;
	bopt.conf_strings["group.id"] = "forwarder-tests-123213ab";
	bopt.conf_ints["receive.message.max.bytes"] = 25100100;
	bopt.address = Tests::main_opt->brokers_as_comma_list();
	Consumer consumer(bopt, get_string(&d0, "topic"));
	thread thr_consumer([&consumer]{
		consumer.run();
	});

	sleep_ms(500);

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

	sleep_ms(500);
	main.mapping_add(d0);

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

	LOG(4, "All done, test exit");
}


TEST_F(Remote_T, simple_f142) {
	Remote_T::simple_f142();
}

}
}
