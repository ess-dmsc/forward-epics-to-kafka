#include <cstdlib>
#include <cstdio>
#include <thread>
#include <vector>
#include <list>
#include <forward_list>
#include <algorithm>
#include <string>
#include <cstring>

#include "logger.h"
#include "configuration.h"
#include "TopicMapping.h"
#include "Config.h"

#include "jansson.h"

#include <unistd.h>

namespace BrightnESS {
namespace ForwardEpicsToKafka {

using std::string;
using std::vector;


/*
TODO
Why not embed a python interpreter?
At least, automatize the parsing and RPC stuff.

{"cmd": "mapping_add", "channel": "IOC:m1.DRBV", "topic": "IOC.m1.DRBV"}
{"cmd": "mapping_add", "channel": "IOC:m3.DRBV", "topic": "IOC.m3.DRBV"}
{"cmd": "mapping_remove_topic", "topic": "IOC.m1.DRBV"}
{"cmd": "mapping_remove_topic", "topic": "IOC.m3.DRBV"}
*/



class Main {
public:
Main() : kafka_instance_set(Kafka::InstanceSet::Set())  { }
void forward_epics_to_kafka(std::string config_file);
void mapping_add(string channel, string topic);
void mapping_start(TopicMappingSettings tmsettings);
void mapping_remove_topic(string topic);
private:
std::list<TopicMappingSettings> tms_to_start;
std::list<std::unique_ptr<TopicMapping>> tms;
std::list<std::unique_ptr<TopicMapping>> tms_failed;

// Keep them until I know how to be sure that Epics will no longer
// invoke any callbacks on them.
std::list<std::unique_ptr<TopicMapping>> tms_zombies;
Kafka::InstanceSet & kafka_instance_set;
friend class ConfigCB;

uint32_t topic_mappings_started = 0;
};



class ConfigCB : public Config::Callback {
public:
ConfigCB(Main & main)
: main(main)
{ }


// This is called from the same thread as the main watchdog below, because the
// code below calls the config poll which in turn calls this callback.

void operator() (std::string const & msg) override {
	LOG(1, "do something...: %s", msg.c_str());

	// TODO
	// Parse JSON commands

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

	// TODO
	// Need a schema parser / validator.
	// Automate the testing if the structure is valid.

	LOG(3, "Got cmd: %s", cmd);

	if (strcmp("mapping_add", cmd) == 0) {
		auto channel = json_string_value(json_object_get(j1, "channel"));
		auto topic   = json_string_value(json_object_get(j1, "topic"));
		main.mapping_add(channel, topic);
	}

	else if (strcmp("mapping_remove_topic", cmd) == 0) {
		auto topic   = json_string_value(json_object_get(j1, "topic"));
		main.mapping_remove_topic(topic);
	}

	else {
	}

	json_decref(j1);

}
private:
Main & main;
};


void Main::forward_epics_to_kafka(std::string config_file) {
	try {
		Config::Listener config_listener({"localhost:9092", "configuration.global"});
		ConfigCB config_cb(*this);

		if (false) {
			vector<vector<string>> names = {
				{"IOC:m1.DRBV", "IOC.m1.DRBV"},
				{"IOC:m2.DRBV", "IOC.m2.DRBV"},
				{"IOC:m3.DRBV", "IOC.m3.DRBV"},
				{"IOC:m4.DRBV", "IOC.m4.DRBV"},
				{"IOC:m5.DRBV", "IOC.m5.DRBV"},
				{"IOC:m6.DRBV", "IOC.m6.DRBV"},
			};
			for (auto & x : names) {
				mapping_add(x.at(0), x.at(1));
			}
		}

		int total_started_setpoint = 9000;

		for (int i1 = 0; i1 < total_started_setpoint; ++i1) {
			char buf1[64], buf2[64];
			//snprintf(buf1, 64, "IOC:m%d.DRBV", i1%6+1);
			//snprintf(buf2, 64, "IOC.m%d.DRBV", i1%6+1);
			snprintf(buf1, 64, "pv.%06d", i1);
			snprintf(buf2, 64, "pv.%06d", i1 < 7 ? i1 : 7);
			mapping_add(buf1, buf2);
		}

		int init_pool_max = 500;

		if (true) {
			int i1 = 0;
			while (true) {

				// House keeping.  After some grace period, clean up the zombies.
				{
					auto & l1 = tms_zombies;
					l1.erase(
						std::remove_if(l1.begin(), l1.end(), [](decltype(tms_zombies)::value_type & x){return x->zombie_can_be_cleaned();}),
						l1.end()
					);
				}

				// Add failed back into the queue
				for (auto & x : tms_failed) {
					mapping_add(x->channel_name(), x->topic_name());
					tms_zombies.push_back(std::move(x));
				}
				tms_failed.clear();

				// keep in this wider scope for later log output:
				int started = 0;

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

				// TODO
				// Collect mappings with issues
				// They should already have stopped their forwarding
				// and be in FAIL.
				// Put them in the list for stats reporting.
				// Only on the next round, try to revive under throttle.
				for (auto & tm : tms) {
					if (tm->health_state() == TopicMapping::State::FAILURE) {
						tms_failed.push_back(std::move(tm));
					}
				}
				tms.erase(std::remove_if(tms.begin(), tms.end(), [](decltype(tms)::value_type & x){return x==0;}), tms.end());


				// TODO
				// Check all Kafka instances.
				// If one dies, we need to remove all TopicMapping from there as well.
				{
					int i1 = 0;
					auto & l1 = kafka_instance_set.instances;
					auto it2 = l1.before_begin();
					for (auto it1 = l1.begin(); it1 != l1.end(); ++it1) {
						if (it1->get()->error_from_kafka_callback_flag) {
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
						// TODO
						// This should not happen because the nullptrs should be
						// already filtered out.
						continue;
					}
					tm->health_selfcheck();
					if (! tm->healthy()) {
						// TODO
						// Remove the unhealthy mappings.
						// Remember the unhealthy mappings in a list.
						// Report to logging.
						LOG(6, "Channel Mapping is not healthy anymore...");
					}
				}

				config_listener.poll(config_cb);

				LOG(1, "running %6d   to_start: %6d   failure %6d   started %5d",
					(int)tms.size(), (int)tms_to_start.size(), (int)tms_failed.size(), started);
				std::this_thread::sleep_for(std::chrono::milliseconds(300));
				if (i1 > 10000) break;
				++i1;
				//exit(1);
			}
		}
	}
	catch (std::runtime_error & e) {
		LOG(6, "CATCH runtime error in main watchdog thread: %s", e.what());
	}
	catch (std::exception & e) {
		LOG(6, "CATCH EXCEPTION in main watchdog thread");
	}

	if (false) {
		// Just wait...
		std::mutex mu1;
		std::condition_variable cv1;
		std::unique_lock<std::mutex> lk(mu1);
		auto x = cv1.wait_for(lk, std::chrono::milliseconds(8000), []{return false;});
		x = !x;
		LOG(0, "after wait");
	}
}


void Main::mapping_add(string channel, string topic) {
	tms_to_start.emplace_back(channel, topic);
}

void Main::mapping_start(TopicMappingSettings tmsettings) {
	auto tm = new TopicMapping(kafka_instance_set, tmsettings, topic_mappings_started);
	topic_mappings_started += 1;
	tms.push_back(decltype(tms)::value_type(tm));
}



void Main::mapping_remove_topic(string topic) {
	for (auto & tm : tms) {
		if (tm->topic_name() == topic) {
			LOG(3, "FOUND");
			tm->stop_forwarding();
		}
	}
}


}
}




int main(int argc, char ** argv) {
	LOG(3, "continue with main c++");
	if (false && argc < 2) {
		LOG(6, "You need to specify the path to the configuration file");
		return 1;
	}
	BrightnESS::ForwardEpicsToKafka::Main main;
	main.forward_epics_to_kafka("NO-CONFIG");
	return 0;
}
