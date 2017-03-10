#include <cstdlib>
#include <cstdio>
#include <thread>
#include <vector>
#include <list>
#include <forward_list>
#include <map>
#include <algorithm>
#include <mutex>
#include <string>
#include <cstring>
#include <csignal>
#include <atomic>

#include <fmt/format.h>
#include "logger.h"
#include "configuration.h"
#include "TopicMapping.h"
#include "Config.h"
#include "MainOpt.h"
#include "KafkaW.h"
#include "git_commit_current.h"
#include "uri.h"
#include "blobs.h"

std::atomic<int> g__run {1};

namespace BrightnESS {
namespace ForwardEpicsToKafka {

class Main {
public:
Main(MainOpt opt);
void forward_epics_to_kafka();
void mapping_add(TopicMappingSettings tmsettings);
void mapping_start(TopicMappingSettings tmsettings);
void mapping_remove_topic(string topic);
void mapping_list();
void forwarding_exit();
void start_some_test_mappings(int n1);
void check_instances();
void collect_and_revive_failed_mappings();
void report_stats(int started_in_current_round);
void start_mappings(int & started);
void move_failed_to_startup_queue();
void release_deleted_mappings();
private:
int const init_pool_max = 64;
bool const do_release_memory = true;
int const memory_release_grace_time = 45;
MainOpt main_opt;
std::list<TopicMappingSettings> tms_to_start;
std::recursive_mutex m_tms_mutex;
std::recursive_mutex m_tms_zombies_mutex;
std::recursive_mutex m_tms_failed_mutex;
std::recursive_mutex m_tms_to_start_mutex;
std::recursive_mutex m_tms_to_delete_mutex;

using RMLG = std::lock_guard<std::recursive_mutex>;
std::list<std::unique_ptr<TopicMapping>> tms;
std::list<std::unique_ptr<TopicMapping>> tms_failed;
std::list<std::unique_ptr<TopicMapping>> tms_to_delete;

// Keep them until I know how to be sure that Epics will no longer
// invoke any callbacks on them.
std::list<std::unique_ptr<TopicMapping>> tms_zombies;
Kafka::InstanceSet & kafka_instance_set;
friend class ConfigCB;

uint32_t topic_mappings_started = 0;
bool forwarding_run = true;
};



class ConfigCB : public Config::Callback {
public:
ConfigCB(Main & main)
: main(main)
{ }


// This is called from the same thread as the main watchdog below, because the
// code below calls the config poll which in turn calls this callback.

void operator() (std::string const & msg) override;
private:
Main & main;
};

void ConfigCB::operator() (std::string const & msg) {
	using std::string;
	using namespace rapidjson;
	LOG(7, "Command received: {}", msg.c_str());
	Document j0;
	j0.Parse(msg.c_str());
	if (j0["cmd"] == "add") {
		auto channel = j0["channel"].GetString();
		auto topic   = j0["topic"].GetString();
		if (channel && topic) {
			//main.mapping_add(channel, topic);
			LOG(0, "ERROR this command handler needs fecatoring, need to look up fb converter first");
		}
	}
	if (j0["cmd"] == "remove") {
		auto channel = j0["channel"].GetString();
		if (channel) {
			main.mapping_remove_topic(channel);
		}
	}
	if (j0["cmd"] == "list") {
		main.mapping_list();
	}
	if (j0["cmd"] == "exit") {
		main.forwarding_exit();
	}
}


Main::Main(MainOpt opt) : main_opt(opt), kafka_instance_set(Kafka::InstanceSet::Set(opt.broker_opt)) {
	if (main_opt.json) {
		if (main_opt.json->HasMember("mappings")) {
			auto & ms = (*main_opt.json)["mappings"];
			if (ms.IsArray()) {
				for (auto & m : ms.GetArray()) {
					auto type_1 = m["type"].GetString();
					if (not type_1) type_1 = "EPICS_PVA_NT";
					string type(type_1);
					/*
					if (type == "chopper") {
						LOG(0, "ERROR NEEDS REFACTOR FIRST");
						throw std::runtime_error("ERROR NEEDS REFACTOR FIRST");
						auto channel = m["channel"].GetString();
						auto topic = m["topic"].GetString();
						if (channel == nullptr or topic == nullptr) {
							LOG(0, "ERROR expect 'channel' and 'topic' configuration settings for a chopper");
							continue;
						}

						// One chopper consists of multiple EPICS PVs.
						// We add thos PVs individually because their are unrelated from EPICS point of view.
						mapping_add({
							TopicMappingType::EPICS_CA_VALUE,
							string(channel) + ".ActSpd",
							string(topic) + ".ActSpd"
						});
						auto tms = TopicMappingSettings({TopicMappingType::EPICS_CA_VALUE,
							string(channel) + ".TDCE",
							string(topic) + ".TDCE"
						});
						tms.is_chopper_TDCE = true;
						mapping_add(tms);
					}
					*/
					if (type == "general" || type == "f140") {
						TopicMappingSettings tms(m["channel"].GetString(), m["topic"].GetString());
						tms.teamid = opt.teamid;
						tms.converter_epics_to_fb = opt.schema_registry.items().at("f140")->create_converter();
						mapping_add(tms);
					}
					if (type == "f141") {
						TopicMappingSettings tms(m["channel"].GetString(), m["topic"].GetString());
						tms.teamid = opt.teamid;
						tms.converter_epics_to_fb = opt.schema_registry.items().at("f141")->create_converter();
						mapping_add(tms);
					}

				}
			}
		}
	}
}



void Main::forward_epics_to_kafka() {
	#define do_config_kafka_listener true

	#if do_config_kafka_listener
		Config::Listener config_listener({
			main_opt.broker_configuration_address,
			main_opt.broker_configuration_topic
		});
		ConfigCB config_cb(*this);
	#endif

	while (forwarding_run and g__run == 1) {
		release_deleted_mappings();
		move_failed_to_startup_queue();
		// keep in this wider scope for later log output:
		int started = 0;
		start_mappings(started);
		collect_and_revive_failed_mappings();
		check_instances();

		#if do_config_kafka_listener
			config_listener.poll(config_cb);
		#endif

		report_stats(started);
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}
}


void Main::release_deleted_mappings() {
	// House keeping.  After some grace period, clean up the zombies.
	if (!do_release_memory) return;

	{
		RMLG lg1(m_tms_zombies_mutex);
		auto & l1 = tms_zombies;
		auto it2 = std::remove_if(l1.begin(), l1.end(), [this](decltype(tms_zombies)::value_type & x){
			return x->zombie_can_be_cleaned(memory_release_grace_time);
		});
		l1.erase(it2, l1.end());
	}

	{
		RMLG lg2(m_tms_to_delete_mutex);
		auto now = std::chrono::system_clock::now();
		auto & l1 = tms_to_delete;
		auto it2 = std::remove_if(l1.begin(), l1.end(), [now, this](decltype(tms_to_delete)::value_type & x){
			auto td = std::chrono::duration_cast<std::chrono::seconds>(now - x->ts_removed).count();
			return td > memory_release_grace_time;
		});
		l1.erase(it2, l1.end());
	}
}




void Main::move_failed_to_startup_queue() {
	// Add failed back into the queue
	for (auto & x : tms_failed) {
		mapping_add(x->topic_mapping_settings);
		if (!x) {
			LOG(2, "ERROR null TopicMapping in tms_failed");
		}
		else {
			tms_zombies.push_back(std::move(x));
		}
	}
	tms_failed.clear();
}



void Main::start_mappings(int & started) {
	RMLG lg1(m_tms_mutex);
	RMLG lg2(m_tms_to_start_mutex);
	if (tms_to_start.size() > 0) {
		// Check how many are currently in their INIT phase:
		int i_INIT = 0;
		for (auto & tm : tms) {
			if (tm->health_state() == TopicMapping::State::INIT) {
				i_INIT += 1;
			}
		}
		int x = std::max(0, init_pool_max - i_INIT);
		int start_max = x;
		auto it1 = tms_to_start.begin();
		int i1 = 0;
		while (i1 < start_max && it1 != tms_to_start.end()) {
			mapping_start(*it1);
			++i1;
			++it1;
		}
		started = i1;
		tms_to_start.erase(tms_to_start.begin(), it1);
	}
}




void Main::check_instances() {
	/*
	Check Kafka instances for failure, and remove in that case.
	Topics hold a sptr to the Instance.  If Instance notices a failure, it will
	also notify the Topics.
	*/
	int i1 = 0;
	auto & l1 = kafka_instance_set.instances;
	auto it2 = l1.before_begin();
	for (auto it1 = l1.begin(); it1 != l1.end(); ++it1) {
		if (it1->get()->instance_failure()) {
			LOG(1, "Instance has issues");
			l1.erase_after(it2);
			it1 = it2;
			continue;
		}
		// Let the TopicMapping discover that the Topic is no longer good
		// and mark itself for removal.
		it1->get()->check_topic_health();
		++i1;
		if (i1 % 10 == 0) std::this_thread::sleep_for(std::chrono::milliseconds(20));
	}
	for (auto & tm : tms) {
		if (!tm) {
			// Should never happen..
			LOG(2, "ERROR nullptr, should never happen");
			continue;
		}
		// TODO
		// Is this still needed?
		tm->health_selfcheck();
	}
}


/**
Collect mappings with issues.
They should already have stopped their forwarding and be in FAIL.
Put them in the list for stats reporting.
Only on the next round, try to revive under throttle.
*/
void Main::collect_and_revive_failed_mappings() {
	RMLG lg1(m_tms_mutex);
	RMLG lg2(m_tms_failed_mutex);
	for (auto & tm : tms) {
		if (tm->health_state() == TopicMapping::State::FAILURE) {
			tms_failed.push_back(std::move(tm));
		}
	}
	auto it2 = std::remove_if(tms.begin(), tms.end(), [](decltype(tms)::value_type & x){
		return x == 0;
	});
	tms.erase(it2, tms.end());
}



void Main::report_stats(int started_in_current_round) {
	RMLG lg1(m_tms_to_start_mutex);
	RMLG lg2(m_tms_to_delete_mutex);
	RMLG lg3(m_tms_zombies_mutex);
	LOG(6, "running {:6}   to_start: {:6}   failure {:6}   started {:5}   to_delete {:5}   zombies {:5}",
		(int)tms.size(),
		(int)tms_to_start.size(),
		(int)tms_failed.size(),
		started_in_current_round,
		(int)tms_to_delete.size(),
		(int)tms_zombies.size()
	);
}





void Main::mapping_add(TopicMappingSettings tmsettings) {
	RMLG lg(m_tms_to_start_mutex);
	tms_to_start.push_back(tmsettings);
}

void Main::mapping_start(TopicMappingSettings tmsettings) {
	RMLG lg1(m_tms_mutex);
	// Only add the mapping if the channel does not yet exist.
	// Otherwise do nothing.
	for (auto & tm : tms) {
		if (tm->channel_name() == tmsettings.channel) return;
	}
	auto tm = new TopicMapping(kafka_instance_set, tmsettings, topic_mappings_started, main_opt.forwarder_ix);
	this->topic_mappings_started += 1;
	tms.push_back(decltype(tms)::value_type(tm));
}



void Main::mapping_remove_topic(string topic) {
	RMLG lg1(m_tms_mutex);
	RMLG lg2(m_tms_to_delete_mutex);
	for (auto & x : tms) {
		if (x->topic_name() == topic) {
			x->stop_forwarding();
			x->ts_removed = std::chrono::system_clock::now();
			tms_to_delete.push_back(std::move(x));
		}
	}
	auto it2 = std::remove_if(tms.begin(), tms.end(), [&topic](std::unique_ptr<TopicMapping> const & x){
		return x.get() == 0;
	});
	tms.erase(it2, tms.end());
}

void Main::mapping_list() {
	RMLG lg1(m_tms_mutex);
	for (auto & tm : tms) {
		printf("%-20s -> %-20s\n", tm->channel_name().c_str(), tm->topic_name().c_str());
	}
}


void Main::forwarding_exit() {
	LOG(2, "exit requested");
	forwarding_run = false;
}


}
}




void signal_handler(int signal) {
	LOG(0, "SIGNAL {}", signal);
	g__run = 0;
}



#if HAVE_GTEST
#include <gtest/gtest.h>
#endif

int main(int argc, char ** argv) {
	if (argc >= 2) {
		if (strcmp("--test-no-opt", argv[1]) == 0) {
			#if HAVE_GTEST
			::testing::InitGoogleTest(&argc, argv);
			return RUN_ALL_TESTS();
			#else
			LOG(4, "You asked for --test-no-opt but this binary is not compiled with gtest");
			return 1;
			#endif
		}
	}
	std::signal(SIGINT, signal_handler);
	std::signal(SIGTERM, signal_handler);
	auto op = BrightnESS::ForwardEpicsToKafka::parse_opt(argc, argv);
	if (op.first) {
		LOG(3, "Could not parse options");
	}
	auto & opt = *op.second;

	if (opt.log_file.size() > 0) {
		use_log_file(opt.log_file);
	}

	fmt::print(
		"forward-epics-to-kafka-0.0.1 {:.7} (ESS, BrightnESS)\n"
		"  Contact: dominik.werder@psi.ch\n\n",
		GIT_COMMIT
	);

	if (opt.help) {
		fmt::print(
			"Forwards EPICS process variables to Kafka topics.\n"
			"Controlled via JSON packets sent over the configuration topic.\n"
			"\n"
			"\n"
			"forward-epics-to-kafka\n"
			"  --help, -h\n"
			"\n"
			"  --config-file                     <file>\n"
			"      Configuration file in JSON format.\n"
			"      To overwrite the options in config-file, specify them later on the command line.\n"
			"\n"
			"  --broker-configuration-address    host:port,host:port,...\n"
			"      Kafka brokers to connect with for configuration updates.\n"
			"      Default: localhost:9092\n"
			"\n"
			"  --broker-configuration-topic      <topic-name>\n"
			"      Topic name to listen to for configuration updates.\n"
			"      Default: configuration.global\n"
			"\n"
			"  --broker-data-address             host:port,host:port,...\n"
			"      Kafka brokers to connect with for configuration updates\n"
			"      Default: localhost:9092\n"
			"\n"
			"  --kafka-gelf                      <kafka://host[:port]/topic>\n"
			"\n"
			"  --graylog-logger-address          <host:port>\n"
			"      Log to Graylog via graylog_logger library.\n"
			"\n"
			"  -v\n"
			"      Decrease log_level by one step.  Default log_level is 3.\n"
			"  -Q\n"
			"      Increase log_level by one step.\n"
			"\n"
		);
		return 1;
	}

	opt.init_after_parse();

	if (opt.kafka_gelf != "") {
		BrightnESS::uri::URI uri(opt.kafka_gelf);
		log_kafka_gelf_start(uri.host, uri.topic);
		LOG(3, "Enabled kafka_gelf: //{}/{}", uri.host, uri.topic);
	}

	if (opt.graylog_logger_address != "") {
		fwd_graylog_logger_enable(opt.graylog_logger_address);
	}

	BrightnESS::ForwardEpicsToKafka::Main main(opt);
	try {
		main.forward_epics_to_kafka();
	}
	catch (std::runtime_error & e) {
		LOG(0, "CATCH runtime error in main watchdog thread: {}", e.what());
	}
	catch (std::exception & e) {
		LOG(0, "CATCH EXCEPTION in main watchdog thread");
	}
	return 0;
}
