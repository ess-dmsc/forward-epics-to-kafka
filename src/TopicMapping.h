#pragma once

#include <memory>
#include <condition_variable>
#include <string>
#include <chrono>
#include <random>

#include "Kafka.h"
#include "fbhelper.h"

namespace BrightnESS {
namespace ForwardEpicsToKafka {

// Forward declare class
namespace Epics {
class Monitor;
}

namespace Kafka {
class Topic;
}



// If we get many different such combinations, maybe better to factor
// this even more, but currently, this seems like the best choice.
enum class TopicMappingType: uint16_t {
	EPICS_PVA_NT,       // currently the default
	EPICS_CA_VALUE,     // TODO do I need an extra waveform, or can that be introspected?
	EPICS_PVA_GENERAL,
};


/**
This class is meant to have trivial copy ctor.
*/
class TopicMappingSettings {
public:
TopicMappingSettings(std::string channel, std::string topic)
:	channel(channel),
	topic(topic)
{ }

TopicMappingSettings(TopicMappingType type, std::string channel, std::string topic)
:	type(type),
	channel(channel),
	topic(topic)
{ }

TopicMappingType type {TopicMappingType::EPICS_PVA_NT};
std::string channel;
std::string topic;

bool is_chopper_TDCE { false };
};


/** \brief
Represents the mapping between a EPICS process variable and a Kafka topic.
*/

class TopicMapping {
public:
using sptr = std::shared_ptr<TopicMapping>;
typedef std::string string;
enum class State { INIT, READY, FAILURE };

//TopicMapping(TopicMapping &&) = default;

/// Defines a mapping, but does not yet start the forwarding
TopicMapping(Kafka::InstanceSet & kset, TopicMappingSettings topic_mapping_settings, uint32_t id);
~TopicMapping();

void start_forwarding(Kafka::InstanceSet & kset);
void stop_forwarding();

void emit(BrightnESS::FlatBufs::FB_uptr fb, uint64_t seq);

/** Called from watchdog thread, opportunity to check own health status */
void health_selfcheck();
bool healthy() const;

string topic_name() const;
string channel_name() const;

State health_state() const;

/** can be called from any thread */
void go_into_failure_mode();

// for debugging:
uint32_t id;

/// Should return true if we waited long enough so that this zombie can be cleaned up
bool zombie_can_be_cleaned(int grace_time);

std::atomic_bool forwarding {true};

// TODO make private
std::chrono::system_clock::time_point ts_removed;

TopicMappingSettings topic_mapping_settings;

private:
// Weak ptr to allow the topic instance go away on failure
std::shared_ptr<Kafka::Topic> topic;
std::condition_variable cv1;
std::mutex mu1;
std::shared_ptr<Epics::Monitor> epics_monitor;
std::chrono::system_clock::time_point ts_init;
std::chrono::system_clock::time_point ts_failure;
State state {State::INIT};

// Variables to produce a verifiable message stream for testing purposes
uint32_t sid = 0;
std::mt19937 rnd;

};

}
}
