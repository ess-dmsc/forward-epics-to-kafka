#include "TopicMapping.h"
#include "logger.h"
#include "epics.h"
#include "Kafka.h"
//#include <mutex>
#include <thread>
#include <array>
#include "tools.h"

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
		LOG(0, "sleeping for %d", x);
		std::this_thread::sleep_for(std::chrono::milliseconds(x));
		if (predicate()) return true;
		i2 *= 2;
	}
	return false;
}


TopicMapping::TopicMapping(Kafka::InstanceSet & kset, TopicMappingSettings topic_mapping_settings, uint32_t id) :
	topic_mapping_settings(topic_mapping_settings),
	ts_init(std::chrono::system_clock::now()),
	id(id)
{
	// If we do not want to start immediately, we should initialize
	// the epics monitor such that it does not yet poll.
	// But we should have the monitor instance set up.
	start_forwarding(kset);
}

TopicMapping::~TopicMapping() {
	LOG(1, "TopicMapping %d dtor", id);
	stop_forwarding();
}


bool TopicMapping::zombie_can_be_cleaned() {
	auto now = std::chrono::system_clock::now();
	return std::chrono::duration_cast<std::chrono::seconds>(now - ts_failure).count() > 10;
}


void TopicMapping::go_into_failure_mode() {
	stop_forwarding();
	state = State::FAILURE;
	ts_failure = std::chrono::system_clock::now();
	//LOG(1, "ts fail: %lu", ts_failure.time_since_epoch().count());
}


void TopicMapping::start_forwarding(Kafka::InstanceSet & kset) {
	// Get an instance from the instance set.
	// The topic should be owned by the instance.
	// We can loan a pointer to the topic as long as we keep the instance alive.
	// But the topic can go away if the instance notices that something bad happened.

	this->topic = std::weak_ptr<Kafka::Topic>(kset.instance()->create_topic(topic_name()));

	LOG(0, "Start Epics monitor for %s", channel_name().c_str());
	epics_monitor.reset(new Epics::Monitor(*this, channel_name()));
}


void TopicMapping::stop_forwarding() {
	// NOTE
	// Can be called even while we are in start_forwarding() in case
	// of an error condition.
	if (epics_monitor) {
		epics_monitor->stop();
	}
}

void TopicMapping::emit(double x) {
	if (state == State::INIT) {
		LOG(0, "not yet ready");
		return;
	}
	if (state != State::READY) {
		LOG(0, "looks like trouble");
		go_into_failure_mode();
		break1();
		return;
	}
	LOG(0, "emitting % e", x);

	auto top = topic.lock();
	if (!top) {
		// Expired pointer should be the only reason why we do not get a lock
		if (!topic.expired()) {
			LOG(9, "WEIRD, shouldnt that be expired?");
		}
		LOG(0, "No producer.  Already dtored?");
		go_into_failure_mode();
		return;
	}

	// TODO
	// Right now, Kafka will take its own copy.  Change in the future.
	std::array<char, 256> buf1;
	int n1 = snprintf(buf1.data(), buf1.size(), "[tm: %6u v: % .3e]", id, x);
	top->produce({buf1.data(), n1});
}


TopicMapping::State TopicMapping::health_state() const { return state; }


void TopicMapping::health_selfcheck() {
	LOG(0, "Current state: %d", state);
	if (state == State::INIT) {
		if (epics_monitor) {
			if (epics_monitor->ready()) {
				state = State::READY;
				return;
			}
		}
		// Check whether we are already too long in INIT state
		auto a = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - ts_init).count();
		if (a > 4) {
			LOG(1, "ERROR init took too long: %d", a);
			go_into_failure_mode();
			return;
		}
	}
	else if (state == State::READY) {
		if (topic.lock() == nullptr && !topic.expired()) {
			LOG(9, "ERROR weak ptr: no lock(), but not expired() either");
		}
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
		if (topic.expired()) {
			LOG(1, "not healthy because topic handle expired");
			go_into_failure_mode();
			return;
		}
	}
	else if (state == State::FAILURE) {
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
