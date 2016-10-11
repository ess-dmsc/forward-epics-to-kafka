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

#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/filereadstream.h>

#ifdef _MSC_VER
	#include "wingetopt.h"
#elif _AIX
	#include <unistd.h>
#else
	#include <getopt.h>
#endif

#include "git_commit_current.h"


int const n_pv_forward = 0;


namespace BrightnESS {
namespace ForwardEpicsToKafka {

using std::string;
using std::vector;



struct MainOpt {
string broker_configuration_address = "localhost:9092";
string broker_configuration_topic = "configuration.global";
string broker_data_address = "localhost:9092";
bool help = false;
string config_file;

void parse_json(string config_file);
};


void MainOpt::parse_json(string config_file) {
	if (config_file == "") {
		LOG(3, "ERROR given config filename is empty");
		return;
	}
	this->config_file = config_file;
	using namespace rapidjson;
	// Parse the JSON configuration and extract parameters.
	// Currently, these parameters take precedence over what is given on the command line.
	FILE * f1 = fopen(config_file.c_str(), "rb");
	int const N1 = 4000;
	char buf1[N1];
	FileReadStream is(f1, buf1, N1);
	Document d;
	d.ParseStream<0, UTF8<>, FileReadStream>(is);
	if (d.HasMember("broker-configuration-address")) {
		broker_configuration_address = d["broker-configuration-address"].GetString();
	}
	if (d.HasMember("broker-configuration-topic")) {
		broker_configuration_topic   = d["broker-configuration-topic"].GetString();
	}
	if (d.HasMember("broker-data-address")) {
		broker_data_address          = d["broker-data-address"].GetString();
	}
}


class Main {
public:
Main(MainOpt opt) : main_opt(opt), kafka_instance_set(Kafka::InstanceSet::Set(opt.broker_data_address)) { }
void forward_epics_to_kafka();
void mapping_add(string channel, string topic);
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
int const memory_release_grace_time = 2;
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
	LOG(0, "Command received: %s", msg.c_str());
	Document j0;
	j0.Parse(msg.c_str());
	if (j0["cmd"] == "add") {
		auto channel = j0["channel"].GetString();
		auto topic   = j0["topic"].GetString();
		if (channel && topic) {
			main.mapping_add(channel, topic);
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
	// Start configuration listener.  It will use its own thread.
	Config::Listener config_listener({
		main_opt.broker_configuration_address,
		main_opt.broker_configuration_topic
	});
	ConfigCB config_cb(*this);

	while (forwarding_run) {
		release_deleted_mappings();
		move_failed_to_startup_queue();
		// keep in this wider scope for later log output:
		int started = 0;
		start_mappings(started);
		collect_and_revive_failed_mappings();
		check_instances();
		config_listener.poll(config_cb);
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
		mapping_add(x->channel_name(), x->topic_name());
		if (!x) {
			LOG(5, "ERROR null TopicMapping in tms_failed");
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
			LOG(6, "Instance has issues");
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
			LOG(5, "ERROR nullptr, should never happen");
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
	LOG(1, "running %6d   to_start: %6d   failure %6d   started %5d   to_delete %5d   zombies %5d",
		(int)tms.size(),
		(int)tms_to_start.size(),
		(int)tms_failed.size(),
		started_in_current_round,
		(int)tms_to_delete.size(),
		(int)tms_zombies.size()
	);
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




#include <configuration.hpp>
#include <redox.hpp>

void test_config_manager() {
	using CM = configuration::communicator::RedisCommunicator;
	using DM = configuration::data::RedisDataManager<CM>;
	using Configuration = configuration::ConfigurationManager<DM,CM>;
	Configuration cs("localhost", 6379, "localhost", 6379);
	cs.Update("some_test2", "kehrgoifd");
}


int main(int argc, char ** argv) {
	//test_config_manager(); return 1;
	BrightnESS::ForwardEpicsToKafka::MainOpt opt;
	static struct option long_options[] = {
		{"help",                            no_argument,              0,  0 },
		{"broker-configuration-address",    required_argument,        0,  0 },
		{"broker-configuration-topic",      required_argument,        0,  0 },
		{"broker-data-address",             required_argument,        0,  0 },
		{"config-file",                     required_argument,        0,  0 },
		{0, 0, 0, 0},
	};
	std::string cmd;
	int option_index = 0;
	bool getopt_error = false;
	while (true) {
		int c = getopt_long(argc, argv, "", long_options, &option_index);
		//LOG(5, "c getopt %d", c);
		if (c == -1) break;
		if (c == '?') {
			//LOG(5, "option argument missing");
			getopt_error = true;
		}
		//printf("at option %s\n", long_options[option_index].name);
		auto lname = long_options[option_index].name;
		switch (c) {
		case 0:
			//LOG(5, "lname: %s", lname);
			// long option without short equivalent:
			if (std::string("help") == lname) {
				opt.help = true;
			}
			if (std::string("config-file") == lname) {
				opt.parse_json(optarg);
			}
			if (std::string("broker-configuration-address") == lname) {
				opt.broker_configuration_address = optarg;
			}
			if (std::string("broker-configuration-topic") == lname) {
				opt.broker_configuration_topic = optarg;
			}
			if (std::string("broker-data-address") == lname) {
				opt.broker_data_address = optarg;
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

	if (getopt_error) {
		LOG(5, "ERROR parsing command line options");
		opt.help = true;
		return 1;
	}

	printf("forward-epics-to-kafka-0.0.1  (ESS, BrightnESS)\n");
	printf("  %s\n", GIT_COMMIT);
	puts("  Contact: dominik.werder@psi.ch");
	puts("");

	if (opt.help) {
		puts("Forwards EPICS process variables to Kafka topics.");
		puts("Controlled via JSON packets sent over the configuration topic.");
		puts("");
		puts("");
		puts("forward-epics-to-kafka");
		puts("  --help");
		puts("");
		puts("  --config-file                     <file>");
		puts("      Configuration file in JSON format.");
		puts("      To overwrite the options in config-file, specify them later on the command line.");
		puts("");
		puts("  --broker-configuration-address    host:port,host:port,...");
		puts("      Kafka brokers to connect with for configuration updates.");
		puts("      Default: localhost:9092");
		puts("");
		puts("  --broker-configuration-topic      <topic-name>");
		puts("      Topic name to listen to for configuration updates.");
		puts("      Default: configuration.global");
		puts("");
		puts("  --broker-data-address             host:port,host:port,...");
		puts("      Kafka brokers to connect with for configuration updates");
		puts("      Default: localhost:9092");
		puts("");
		return 1;
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
