#include "Main.h"
#include "helper.h"
#include "logger.h"
#include "Config.h"
#include "Stream.h"
#include "ForwarderInfo.h"
#include <sys/types.h>
#include <unistd.h>

namespace BrightnESS {
namespace ForwardEpicsToKafka {

// Little helper
static KafkaW::BrokerOpt make_broker_opt(MainOpt const & opt) {
	KafkaW::BrokerOpt ret = opt.broker_opt;
	ret.address = opt.brokers_as_comma_list();
	return ret;
}


/**
\class Main
\brief Main program entry class.
*/
Main::Main(MainOpt & opt) : 
		main_opt(opt),
		kafka_instance_set(Kafka::InstanceSet::Set(make_broker_opt(opt))),
		conversion_scheduler(this)
{
	finfo = std::shared_ptr<ForwarderInfo>(new ForwarderInfo(this));
	finfo->teamid = main_opt.teamid;

	for (int i1 = 0; i1 < opt.conversion_threads; ++i1) {
		conversion_workers.emplace_back(new ConversionWorker(&conversion_scheduler, opt.conversion_worker_queue_size));
	}

	bool use_config = true;
	if (main_opt.broker_config.topic.size() == 0) {
		LOG(3, "Name for configuration topic is empty");
		use_config = false;
	}
	if (main_opt.broker_config.host.size() == 0) {
		LOG(3, "Host for configuration topic broker is empty");
		use_config = false;
	}
	if (use_config) {
		KafkaW::BrokerOpt bopt;
		bopt.conf_strings["group.id"] = fmt::format("forwarder-command-listener--pid{}", getpid());
		config_listener.reset(new Config::Listener {bopt, main_opt.broker_config});
	}
	if (main_opt.json) {
		auto m1 = main_opt.json->FindMember("streams");
		if (m1 != main_opt.json->MemberEnd()) {
			if (m1->value.IsArray()) {
				for (auto & m : m1->value.GetArray()) {
					mapping_add(m);
				}
			}
		}
	}
}



Main::~Main() {
	LOG(7, "~Main");
	streams_clear();
	conversion_workers_clear();
}



/**
\brief Helper class to provide a callback for the Kafka command listener.
*/
class ConfigCB : public Config::Callback {
public:
ConfigCB(Main & main);
// This is called from the same thread as the main watchdog below, because the
// code below calls the config poll which in turn calls this callback.
void operator() (std::string const & msg) override;
private:
Main & main;
};

ConfigCB::ConfigCB(Main & main) : main(main) { }

void ConfigCB::operator() (std::string const & msg) {
	using std::string;
	using namespace rapidjson;
	LOG(7, "Command received: {}", msg.c_str());
	Document j0;
	j0.Parse(msg.c_str());
	if (j0.HasParseError()) {
		return;
	}
	auto m1 = j0.FindMember("cmd");
	if (m1 == j0.MemberEnd()) {
		return;
	}
	if (!m1->value.IsString()) {
		return;
	}
	string cmd = m1->value.GetString();
	if (cmd == "add") {
		auto m2 = j0.FindMember("streams");
		if (m2 == j0.MemberEnd()) {
			return;
		}
		if (m2->value.IsArray()) {
			for (auto & x : m2->value.GetArray()) {
				main.mapping_add(x);
			}
		}
	}
	if (cmd == "exit") {
		main.forwarding_exit();
	}
}


int Main::streams_clear() {
	CLOG(7, 1, "Main::streams_clear()  begin");
	std::unique_lock<std::mutex> lock(streams_mutex);
	if (streams.size() > 0) {
		for (auto & x : streams) {
			x->stop();
		}
		// Wait for Epics to cool down
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		streams.clear();
	}
	CLOG(7, 1, "Main::streams_clear()  end");
	return 0;
}


int Main::conversion_workers_clear() {
	CLOG(7, 1, "Main::conversion_workers_clear()  begin");
	std::unique_lock<std::mutex> lock(conversion_workers_mx);
	if (conversion_workers.size() > 0) {
		for (auto & x : conversion_workers) {
			x->stop();
		}
		conversion_workers.clear();
	}
	CLOG(7, 1, "Main::conversion_workers_clear()  end");
	return 0;
}


std::unique_lock<std::mutex> Main::get_lock_streams() {
	return std::unique_lock<std::mutex>(streams_mutex);
}


/**
\brief Main program loop.

Start conversion worker threads, poll for command sfrom Kafka.
When stop flag raised, clear all workers and streams.
*/
void Main::forward_epics_to_kafka() {
	using CLK = std::chrono::steady_clock;
	using MS = std::chrono::milliseconds;
	auto Dt = MS(1000);
	ConfigCB config_cb(*this);
	{
		std::unique_lock<std::mutex> lock(conversion_workers_mx);
		for (auto & x : conversion_workers) {
			x->start();
		}
	}
	while (forwarding_run.load() == 1) {
		auto t1 = CLK::now();
		if (config_listener) config_listener->poll(config_cb);
		kafka_instance_set->poll();

		auto t2 = CLK::now();
		auto dt = std::chrono::duration_cast<MS>(t2-t1);
		report_stats(dt.count());
		if (dt >= Dt) {
			CLOG(3, 1, "slow main loop");
		}
		std::this_thread::sleep_for(Dt-dt);
	}
	LOG(7, "Main::forward_epics_to_kafka   shutting down");
	conversion_workers_clear();
	streams_clear();
	LOG(7, "ForwardingStatus::STOPPED");
	forwarding_status.store(ForwardingStatus::STOPPED);
}


void Main::report_stats(int dt) {
	auto m1 = g__total_msgs_to_kafka.load();
	auto m2 = m1 / 1000;
	m1 = m1 % 1000;
	uint64_t b1 = g__total_bytes_to_kafka.load();
	auto b2 = b1 / 1024;
	b1 %= 1024;
	auto b3 = b2 / 1024;
	b2 %= 1024;
	CLOG(6, 5, "dt: {:4}  m: {:4}.{:03}  MB: {:3}.{:03}.{:03}", dt, m2, m1, b3, b2, b1);
}


int Main::mapping_add(rapidjson::Value & mapping) {
	using std::string;
	string channel = get_string(&mapping, "channel");
	if (channel.size() == 0) {
		LOG(3, "mapping channel is not specified");
		return -1;
	}
	std::unique_lock<std::mutex> lock(streams_mutex);
	streams.emplace_back(new Stream(finfo, {channel}));
	auto & stream = streams.back();
	if (main_opt.teamid != 0) {
		stream->teamid(main_opt.teamid);
	}
	{
		auto push_conv = [this, &stream] (rapidjson::Value & c) {
			string schema = get_string(&c, "schema");
			string topic = get_string(&c, "topic");
			if (schema.size() == 0) {
				LOG(3, "mapping schema is not specified");
			}
			if (topic.size() == 0) {
				LOG(3, "mapping topic is not specified");
			}
			auto r1 = main_opt.schema_registry.items().find(schema);
			if (r1 == main_opt.schema_registry.items().end()) {
				LOG(3, "can not handle (yet?) schema id {}", schema);
			}
			uri::URI uri;
			if (main_opt.brokers.size() > 0) {
				uri = main_opt.brokers.at(0);
			}
			uri.topic = topic;
			stream->converter_add(*kafka_instance_set, main_opt.schema_registry, schema, uri);
		};
		auto mconv = mapping.FindMember("converter");
		if (mconv != mapping.MemberEnd()) {
			auto & conv = mconv->value;
			if (conv.IsObject()) {
				push_conv(conv);
			}
			else if (conv.IsArray()) {
				for (auto & c : conv.GetArray()) {
					push_conv(c);
				}
			}
		}
	}
	return 0;
}




std::atomic<uint64_t> g__total_msgs_to_kafka {0};
std::atomic<uint64_t> g__total_bytes_to_kafka {0};

void Main::forwarding_exit() {
	forwarding_run.store(0);
}

}
}
