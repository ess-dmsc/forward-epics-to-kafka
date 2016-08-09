#include <cstdlib>
#include <cstdio>
#include <thread>
#include <vector>
#include <list>
#include <forward_list>
#include <algorithm>
#include <mutex>
#include <string>
#include <cstring>

#include "logger.h"
#include "configuration.h"
#include "TopicMapping.h"
#include "Config.h"

#include "jansson.h"

#include <unistd.h>
#include <getopt.h>


int const n_pv_forward = 0;


namespace BrightnESS {
namespace ForwardEpicsToKafka {

using std::string;
using std::vector;



struct MainOpt {
string broker_configuration_address = "localhost:9092";
string broker_configuration_topic = "configuration.global";
};


class Main {
public:
Main(MainOpt opt) : main_opt(opt), kafka_instance_set(Kafka::InstanceSet::Set()) { }
void forward_epics_to_kafka();
void mapping_add(string channel, string topic);
void mapping_start(TopicMappingSettings tmsettings);
void mapping_remove_topic(string topic);
void mapping_list();
void forwarding_exit();
void start_some_test_mappings(int n1);
private:
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
	LOG(0, "Command received: %s", msg.c_str());

	auto j1 = json_loads(msg.c_str(), 0, nullptr);
	if (!j1) {
		LOG(3, "error can not parse json");
		return;
	}

	auto j_cmd = json_object_get(j1, "cmd");
	if (!j_cmd) {
		LOG(3, "error payload has no cmd");
		return;
	}

	auto cmd = json_string_value(j_cmd);
	if (!cmd) {
		LOG(3, "error cmd has no value!");
		return;
	}

	if (string("add") == cmd) {
		auto channel = json_string_value(json_object_get(j1, "channel"));
		auto topic   = json_string_value(json_object_get(j1, "topic"));
		main.mapping_add(channel, topic);
	}

	else if (string("remove") == cmd) {
		auto channel = json_string_value(json_object_get(j1, "channel"));
		main.mapping_remove_topic(channel);
	}

	else if (string("list") == cmd) {
		main.mapping_list();
	}

	else if (string("exit") == cmd) {
		main.forwarding_exit();
	}

	else {
	}

	json_decref(j1);
}



/*
Start some default mappings for testing purposes.
*/
void Main::start_some_test_mappings(int n1) {
	for (int i1 = 0; i1 < n1; ++i1) {
		int const N1 = 32;
		char buf1[N1], buf2[N1];
		snprintf(buf1, N1, "pv.%06d", i1);
		snprintf(buf2, N1, "pv.%06d", i1 < 4 ? i1 : 4);
		mapping_add(buf1, buf2);
	}
}


void Main::forward_epics_to_kafka() {
	bool do_release_memory = true;
	int memory_release_grace_time = 40;

	Config::Listener config_listener({main_opt.broker_configuration_address, main_opt.broker_configuration_topic});
	ConfigCB config_cb(*this);

	int init_pool_max = 64;

	while (forwarding_run) {

		// House keeping.  After some grace period, clean up the zombies.
		if (do_release_memory) {
			RMLG lg2(m_tms_zombies_mutex);
			auto & l1 = tms_zombies;
			auto it2 = std::remove_if(l1.begin(), l1.end(), [memory_release_grace_time](decltype(tms_zombies)::value_type & x){
				return x->zombie_can_be_cleaned(memory_release_grace_time);
			});
			l1.erase(it2, l1.end());
		}

		if (do_release_memory) {
			RMLG lg2(m_tms_to_delete_mutex);
			auto now = std::chrono::system_clock::now();
			auto & l1 = tms_to_delete;
			auto it2 = std::remove_if(l1.begin(), l1.end(), [now, memory_release_grace_time](decltype(tms_to_delete)::value_type & x){
				auto td = std::chrono::duration_cast<std::chrono::seconds>(now - x->ts_removed).count();
				return td > memory_release_grace_time;
			});
			l1.erase(it2, l1.end());
		}

		// Add failed back into the queue
		for (auto & x : tms_failed) {
			mapping_add(x->channel_name(), x->topic_name());
			if (not x) {
				LOG(5, "ERROR null TopicMapping in tms_failed");
			}
			else {
				tms_zombies.push_back(std::move(x));
			}
		}
		tms_failed.clear();

		// keep in this wider scope for later log output:
		int started = 0;

		{
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

		// TODO
		// Collect mappings with issues
		// They should already have stopped their forwarding
		// and be in FAIL.
		// Put them in the list for stats reporting.
		// Only on the next round, try to revive under throttle.
		{
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


		// TODO
		// Check all Kafka instances.
		// If one dies, we need to remove all TopicMapping from there as well.
		/*
		-	Remove all affected topic mappings (mark them failed via the usual mechanism)
		-	TopicMappings hold a shared pointer to the Instance
			Therefore, Instance will be only dtored after all Topics are gone.
		*/
		{
			int i1 = 0;
			auto & l1 = kafka_instance_set.instances;
			auto it2 = l1.before_begin();
			for (auto it1 = l1.begin(); it1 != l1.end(); ++it1) {
				if (it1->get()->instance_failure()) {
					LOG(6, "Instance has issues");
					l1.erase_after(it2);
					it1 = it2;
					continue;
				}
				// TODO
				// Let the instance check the topics.
				// The topic should be marked.
				// But if the instance itself is not affected, do not destroy it!

				// Let the TopicMapping discover that the Topic is no longer good
				// and mark itself for removal.
				it1->get()->check_topic_health();
				++i1;
				if (i1 % 10 == 0) std::this_thread::sleep_for(std::chrono::milliseconds(20));
			}
		}
		for (auto & tm : tms) {
			if (!tm) {
				// Should never happen..
				LOG(5, "ERROR nullptr, should never happen");
				continue;
			}
			// TODO
			// Is this still needed?
			tm->health_selfcheck();
		}

		config_listener.poll(config_cb);

		{
			RMLG lg1(m_tms_to_start_mutex);
			RMLG lg2(m_tms_to_delete_mutex);
			RMLG lg3(m_tms_zombies_mutex);
			LOG(1, "running %6d   to_start: %6d   failure %6d   started %5d   to_delete %5d   zombies %5d",
				(int)tms.size(),
				(int)tms_to_start.size(),
				(int)tms_failed.size(),
				started,
				(int)tms_to_delete.size(),
				(int)tms_zombies.size()
			);
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}
}



void Main::mapping_add(string channel, string topic) {
	RMLG lg(m_tms_to_start_mutex);
	tms_to_start.emplace_back(channel, topic);
}

void Main::mapping_start(TopicMappingSettings tmsettings) {
	RMLG lg1(m_tms_mutex);
	// Only add the mapping if the channel does not yet exist.
	// Otherwise do nothing.
	for (auto & tm : tms) {
		if (tm->channel_name() == tmsettings.channel) return;
	}
	auto tm = new TopicMapping(kafka_instance_set, tmsettings, topic_mappings_started);
	topic_mappings_started += 1;
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
	LOG(5, "exit requested");
	forwarding_run = false;
}


}
}




int main(int argc, char ** argv) {
	BrightnESS::ForwardEpicsToKafka::MainOpt opt;
	static struct option long_options[] = {
		{"broker-configuration-address",    required_argument,        0,  0 },
		{"broker-configuration-topic",      required_argument,        0,  0 },
		{0, 0, 0, 0},
	};
	std::string cmd;
	int option_index = 0;
	while (true) {
		int c = getopt_long(argc, argv, "", long_options, &option_index);
		if (c == -1) break;
		//printf("at option %s\n", long_options[option_index].name);
		auto lname = long_options[option_index].name;
		switch (c) {
		case 0:
			// long option without short equivalent:
			if (std::string("broker-configuration-address") == lname) {
				opt.broker_configuration_address = optarg;
			}
			if (std::string("broker-configuration-topic") == lname) {
				opt.broker_configuration_topic = optarg;
			}
		}
		// TODO catch error from missing argument
		// TODO print a command help
	}
	if (optind < argc) {
		printf("Left-over commandline options:\n");
		for (int i1 = optind; i1 < argc; ++i1) {
			printf("%2d %s\n", i1, argv[i1]);
		}
	}
	BrightnESS::ForwardEpicsToKafka::Main main(opt);
	try {
		main.forward_epics_to_kafka();
	}
	catch (std::runtime_error & e) {
		LOG(6, "CATCH runtime error in main watchdog thread: %s", e.what());
	}
	catch (std::exception & e) {
		LOG(6, "CATCH EXCEPTION in main watchdog thread");
	}
	return 0;
}
