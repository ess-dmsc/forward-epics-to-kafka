#include "Main.h"
#include "logger.h"
#include "Config.h"
#include "helper.h"
//#include "KafkaW.h"
//#include "uri.h"

namespace BrightnESS {
namespace ForwardEpicsToKafka {

std::atomic<int> g__run {1};

KafkaW::BrokerOpt make_broker_opt(MainOpt const & opt) {
	KafkaW::BrokerOpt ret = opt.broker_opt;
	ret.address = opt.brokers_as_comma_list();
	return ret;
}

Main::Main(MainOpt & opt) : main_opt(opt), kafka_instance_set(Kafka::InstanceSet::Set(make_broker_opt(opt))) {
	if (main_opt.json) {
		auto m1 = main_opt.json->FindMember("mappings");
		if (m1 != main_opt.json->MemberEnd()) {
			if (m1->value.IsArray()) {
				for (auto & m : m1->value.GetArray()) {
					mapping_add(m);
				}
			}
		}
	}
}



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



void Main::forward_epics_to_kafka() {
	KafkaW::BrokerOpt bopt;
	bopt.conf_strings["group.id"] = "forwarder-command-listener";
	Config::Listener config_listener(bopt, main_opt.broker_config);
	ConfigCB config_cb(*this);

	while (forwarding_run == 1) {
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


int Main::mapping_add(rapidjson::Value & mapping) {
	// TODO Validate
	using std::string;
	string type = get_string(&mapping, "type");
	string channel = get_string(&mapping, "channel");
	string topic = get_string(&mapping, "topic");
	if (type.size() == 0) {
		LOG(3, "mapping type is not specified");
		return -1;
	}
	if (channel.size() == 0) {
		LOG(3, "mapping channel is not specified");
		return -1;
	}
	if (topic.size() == 0) {
		LOG(3, "mapping topic is not specified");
		return -1;
	}
	auto r1 = main_opt.schema_registry.items().find(type);
	if (r1 == main_opt.schema_registry.items().end()) {
		LOG(3, "can not handle (yet?) schema id {}", type);
		return -2;
	}
	TopicMappingSettings tms(channel, topic);
	tms.teamid = main_opt.teamid;
	tms.converter_epics_to_fb = r1->second->create_converter();
	mapping_add(tms);
	return 0;
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
	auto it2 = std::remove_if(tms.begin(), tms.end(), [](std::unique_ptr<TopicMapping> const & x){
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
	LOG(4, "exit requested");
	forwarding_run = 0;
}


}
}
