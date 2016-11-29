#include "TopicMapping.h"
#include "logger.h"
#include "epics.h"
#include "Kafka.h"
//#include <mutex>
#include <thread>
#include <array>
#include "tools.h"
#include "helper.h"

namespace BrightnESS {
namespace ForwardEpicsToKafka {

void some_test() {
	LOG(0, "lock mutex first time");
	std::timed_mutex m1;
	m1.lock();
	LOG(0, "try locking 2nd time");
	m1.try_lock_for(std::chrono::milliseconds(1000));
	LOG(0, "adter locking try...");
}


bool wait(int time, std::function<bool()> predicate) {
	int const n = 6;
	int const fr = pow(2, n+1) - 1;
	int i2 = 1;
	for (int i1 = 0; i1 < n; ++i1) {
		int x = i2 * time / fr + 1;
		LOG(0, "sleeping for {}", x);
		std::this_thread::sleep_for(std::chrono::milliseconds(x));
		if (predicate()) return true;
		i2 *= 2;
	}
	return false;
}


TopicMapping::TopicMapping(Kafka::InstanceSet & kset, TopicMappingSettings topic_mapping_settings, uint32_t id) :
	id(id),
	topic_mapping_settings(topic_mapping_settings),
	ts_init(std::chrono::system_clock::now()),
	rnd((uint32_t)id)
{
	// If we do not want to start immediately, we should initialize
	// the epics monitor such that it does not yet poll.
	// But we should have the monitor instance set up.
	start_forwarding(kset);
}

TopicMapping::~TopicMapping() {
	LOG(1, "TopicMapping {} dtor", id);
	if (epics_monitor) {
		epics_monitor->topic_mapping_gone();
	}
	stop_forwarding();
}


bool TopicMapping::zombie_can_be_cleaned(int grace_time) {
	auto now = std::chrono::system_clock::now();
	return std::chrono::duration_cast<std::chrono::seconds>(now - ts_failure).count() > grace_time;
}


void TopicMapping::go_into_failure_mode() {
	LOG(3, "failure mode for topic mapping {}", id);
	stop_forwarding();
	state = State::FAILURE;
	ts_failure = std::chrono::system_clock::now();
	//LOG(1, "ts fail: {}", ts_failure.time_since_epoch().count());
}


void TopicMapping::start_forwarding(Kafka::InstanceSet & kset) {
	// Get an instance from the instance set.
	// The topic should be owned by the instance.
	// We can loan a pointer to the topic as long as we keep the instance alive.
	// But the topic can go away if the instance notices that something bad happened.

	//this->topic = std::weak_ptr<Kafka::Topic>(kset.instance()->get_or_create_topic(topic_name()));
	this->topic = kset.instance()->get_or_create_topic(topic_name());

	LOG(0, "Start Epics monitor for {}", channel_name().c_str());
	epics_monitor.reset(new Epics::Monitor(this, channel_name()));
	epics_monitor->init(epics_monitor);
}


void TopicMapping::stop_forwarding() {
	// NOTE
	// Can be called even while we are in start_forwarding() in case
	// of an error condition.
	forwarding = false;
	if (epics_monitor) {
		epics_monitor->stop();
	}
}

void TopicMapping::emit(BrightnESS::FlatBufs::FB_uptr fb) {
	if (!forwarding) {
		LOG(3, "WARNING emit called despite not forwarding");
		return;
	}
	if (state == State::INIT) {
		LOG(0, "not yet ready");
		return;
	}
	if (state != State::READY) {
		LOG(3, "looks like trouble");
		go_into_failure_mode();
		return;
	}

	if (!topic->healthy()) {
		go_into_failure_mode();
		return;
	}

	auto & top = topic;

	bool debug_messages = false;
	if (!debug_messages) {
		//LOG(5, "TM {} producing size {}", id, fb->builder->GetSize());
		// Produce the actual payload
		//top->produce({reinterpret_cast<char*>(fb->builder->GetBufferPointer()), fb->builder->GetSize()});
		top->produce(std::move(fb));
		//auto hex = binary_to_hex(reinterpret_cast<char*>(fb->builder->GetBufferPointer()), fb->builder->GetSize());
		//LOG(5, "packet in hex {}: {:.{}}", fb->builder->GetSize(), hex.data(), hex.size());
	}

	else {
		// What follows is testing code to produce a pseudo random sequence
		// which can be checked in the verification end.

		// TODO
		// Right now, Kafka will take its own copy.  Change in the future.
		static std::vector<char> buf1(8*1024, '-');
		static std::vector<char> buf2(8*1024, '-');
		if (true) {
			auto prod = [this, &top](uint32_t sid, uint32_t val) {
				auto n1 = snprintf(buf1.data(), buf1.size(), "sid %12u val %12u  topic %32.32s", sid, val, topic_name().c_str());
				// Artifical payload:
				*(buf1.data()+n1) = '-';
				n1 = 4 * 1024;

				// Right now, can not produce because we limit to FBBptr because of memory management
				// TODO
				//top->produce({buf1.data(), size_t(n1)});


				//LOG(3, "prod: {}", buf1.data());
			};
			if (sid == 0) {
				// send initializer first.  Kafka fortunately guarantees order within a partition.
				prod(0, 0);
			}
			uint32_t val;
			while (true) { val = rnd(); if (val != 0) break; }
			if (false) {
				// Provoke some error:
				if (sid > 1000) val += 1;
			}
			prod(sid, val);
			sid += 1;
		}
	}
}


TopicMapping::State TopicMapping::health_state() const { return state; }


void TopicMapping::health_selfcheck() {
	if (topic) {
		LOG(0, "topic.healthy(): {}", topic->healthy());
	}
	else {
		LOG(0, "topic == nullptr");
	}
	if (state == State::INIT) {
		LOG(0, "still in INIT");
		if (epics_monitor) {
			if (epics_monitor->ready()) {
				state = State::READY;
				return;
			}
		}
		// Check whether we are already too long in INIT state
		auto a = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - ts_init).count();
		if (a > 4) {
			LOG(1, "ERROR init took too long: {}", a);
			go_into_failure_mode();
			return;
		}
	}
	else if (state == State::READY) {
		if (!epics_monitor) {
			LOG(1, "not healthy because we have no epics monitor");
			go_into_failure_mode();
			return;
		}
		if (!epics_monitor->ready()) {
			LOG(1, "not healthy because monitor not ready");
			go_into_failure_mode();
			return;
		}
		if (!topic->healthy()) {
			LOG(1, "not healthy because topic handle expired");
			go_into_failure_mode();
			return;
		}
	}
	else if (state == State::FAILURE) {
		LOG(1, "in FAILURE");
		return;
	}
}


bool TopicMapping::healthy() const {
	return state != State::FAILURE;
}



std::string TopicMapping::topic_name() const { return topic_mapping_settings.topic; }
std::string TopicMapping::channel_name() const { return topic_mapping_settings.channel; }

}
}
