/** \file
Implements the requester classes that Epics uses as callbacks.
Be careful with exceptions as the Epics C++ API does not clearly state
from which threads the callbacks are invoked.
Instead, we call the supervising class which in turn stops operation
and marks the mapping as failed.  Cleanup is then triggered in the main
watchdog thread.
*/

#include "epics.h"
#include "epics-to-fb.h"
#include "logger.h"
#include "TopicMapping.h"
#include <exception>
#include <random>
#include <type_traits>
#include <chrono>
#include "local_config.h"
#include "helper.h"
#include <vector>

// For NTNDArray tests:
#include <pv/nt.h>
#include <pv/ntndarray.h>
#include <pv/ntndarrayAttribute.h>
#include <pv/ntutils.h>

#include "fbhelper.h"
#include "fbschemas.h"

namespace BrightnESS {
namespace ForwardEpicsToKafka {

namespace Epics {

using std::string;
using epics::pvData::Structure;
using epics::pvData::PVStructure;
using epics::pvData::Field;
using epics::pvData::MessageType;
using epics::pvAccess::Channel;

#define STRINGIFY2(x) #x
#define STRINGIFY(x) STRINGIFY2(x)

char const * channel_state_name(epics::pvAccess::Channel::ConnectionState x) {
#define DWTN1(N) DWTN2(N, STRINGIFY(N))
#define DWTN2(N, S) if (x == epics::pvAccess::Channel::ConnectionState::N) { return S; }
	DWTN1(NEVER_CONNECTED);
	DWTN1(CONNECTED);
	DWTN1(DISCONNECTED);
	DWTN1(DESTROYED);
#undef DWTN1
#undef DWTN2
	return "[unknown]";
}




class ActionOnChannel {
public:
ActionOnChannel(Monitor::wptr monitor) : monitor(monitor) { }
virtual void operator () (epics::pvAccess::Channel::shared_pointer const & channel) {
	LOG(2, "[EMPTY ACTION]");
};
Monitor::wptr monitor;
};



//                ChannelRequester

// Listener for channel state changes
class ChannelRequester : public epics::pvAccess::ChannelRequester {
public:
ChannelRequester(std::unique_ptr<ActionOnChannel> action);

// From class pvData::Requester
std::string getRequesterName() override;
void message(std::string const & message, MessageType messageType) override;

void channelCreated(const epics::pvData::Status& status, epics::pvAccess::Channel::shared_pointer const & channel) override;
void channelStateChange(epics::pvAccess::Channel::shared_pointer const & channel, epics::pvAccess::Channel::ConnectionState connectionState) override;

private:
//GetFieldRequesterDW::shared_pointer gfr;
//epics::pvAccess::ChannelGetRequester::shared_pointer cgr;
//ChannelGet::shared_pointer cg;

std::unique_ptr<ActionOnChannel> action;

// Monitor operation:
epics::pvData::MonitorRequester::shared_pointer monr;
epics::pvData::MonitorPtr mon;
};





ChannelRequester::ChannelRequester(std::unique_ptr<ActionOnChannel> action) :
	action(std::move(action))
{
}

string ChannelRequester::getRequesterName() {
	return "ChannelRequester";
}

void ChannelRequester::message(std::string const & message, MessageType messageType) {
	LOG(4, "Message for: {}  msg: {}  msgtype: {}", getRequesterName().c_str(), message.c_str(), getMessageTypeName(messageType).c_str());
}



/*
Seems that channel creation is actually a synchronous operation
and that this requester callback is called from the same stack
from which the channel creation was initiated.
*/

void ChannelRequester::channelCreated(epics::pvData::Status const & status, Channel::shared_pointer const & channel) {
	auto monitor = action->monitor.lock();
	if (not monitor) {
		LOG(0, "ERROR Assertion failed:  Expect to get a shared_ptr to the monitor");
	}
	#if TEST_RANDOM_FAILURES
		std::mt19937 rnd(std::chrono::system_clock::now().time_since_epoch().count());
		if (rnd() < 0.1 * 0xffffffff) {
			if (monitor) monitor->go_into_failure_mode();
		}
	#endif
	LOG(7, "ChannelRequester::channelCreated:  (int)status.isOK(): {}", (int)status.isOK());
	if (!status.isOK() or !status.isSuccess()) {
		// quick fix until decided on logging system..
		std::ostringstream s1;
		s1 << status;
		LOG(4, "WARNING ChannelRequester::channelCreated:  {}", s1.str().c_str());
	}
	if (!status.isSuccess()) {
		// quick fix until decided on logging system..
		std::ostringstream s1;
		s1 << status;
		LOG(1, "ChannelRequester::channelCreated:  failure: {}", s1.str().c_str());
		if (channel) {
			// Yes, take a copy
			std::string cname = channel->getChannelName();
			LOG(1, "  failure is in channel: {}", cname.c_str());
		}
		if (monitor) monitor->go_into_failure_mode();
	}
}

void ChannelRequester::channelStateChange(Channel::shared_pointer const & channel, Channel::ConnectionState cstate) {
	auto monitor = action->monitor.lock();
	LOG(7, "channel state change: {}", Channel::ConnectionStateNames[cstate]);
	if (cstate == Channel::DISCONNECTED) {
		LOG(6, "Epics channel disconnect");
		if (monitor) monitor->go_into_failure_mode();
		return;
	}
	else if (cstate == Channel::DESTROYED) {
		LOG(6, "Epics channel destroyed");
		if (monitor) monitor->go_into_failure_mode();
		return;
	}
	else if (cstate != Channel::CONNECTED) {
		LOG(1, "Unhandled channel state change: {}", channel_state_name(cstate));
		if (monitor) monitor->go_into_failure_mode();
	}
	if (!channel) {
		LOG(1, "ERROR no channel, even though we should have.  state: {}", channel_state_name(cstate));
		if (monitor) monitor->go_into_failure_mode();
	}

	action->operator()(channel);
}




class ActionOnField {
public:
ActionOnField(Monitor::wptr monitor) : monitor(monitor) { }
virtual void operator () (Field const & field) { };
Monitor::wptr monitor;
};



class GetFieldRequesterForAction : public epics::pvAccess::GetFieldRequester {
public:
GetFieldRequesterForAction(epics::pvAccess::Channel::shared_pointer channel, std::unique_ptr<ActionOnField> action);
// from class epics::pvData::Requester
string getRequesterName() override;
void message(string const & msg, epics::pvData::MessageType msgT) override;
void getDone(epics::pvData::Status const & status, epics::pvData::FieldConstPtr const & field) override;
private:
std::unique_ptr<ActionOnField> action;
};

GetFieldRequesterForAction::GetFieldRequesterForAction(epics::pvAccess::Channel::shared_pointer channel, std::unique_ptr<ActionOnField> action) :
	action(std::move(action))
{
	LOG(7, STRINGIFY(GetFieldRequesterForAction) " ctor");
}

string GetFieldRequesterForAction::getRequesterName() { return STRINGIFY(GetFieldRequesterForAction); }

void GetFieldRequesterForAction::message(string const & msg, epics::pvData::MessageType msgT) {
	LOG(4, "GetFieldRequesterForAction::message: {}", msg.c_str());
}

void GetFieldRequesterForAction::getDone(epics::pvData::Status const & status, epics::pvData::FieldConstPtr const & field) {
	if (!status.isSuccess()) {
		LOG(1, "ERROR nosuccess");
		auto monitor = action->monitor.lock();
		if (monitor) {
			monitor->go_into_failure_mode();
		}
	}
	if (status.isOK()) {
		LOG(7, "success and OK");
	}
	else {
		LOG(4, "success with warning:  [[TODO STATUS]]");
	}
	action->operator()(*field);
}





// ============================================================================
// MONITOR
// The Monitor class is in pvData, they say that's because it only depends on pvData, so they have put
// it there, what a logic...
// Again, we have a MonitorRequester and :


class MonitorRequester : public ::epics::pvData::MonitorRequester {
public:
// TODO should also have a monitorrequester_ID for testing purposes.
/// Keeps track of how many events arrived at this MonitorRequester
uint64_t seq = 0;

MonitorRequester(std::string channel_name, Monitor::wptr monitor_HL);
~MonitorRequester();
string getRequesterName() override;
void message(string const & msg, epics::pvData::MessageType msgT) override;

void monitorConnect(epics::pvData::Status const & status, epics::pvData::Monitor::shared_pointer const & monitor, epics::pvData::StructureConstPtr const & structure) override;

void monitorEvent(epics::pvData::MonitorPtr const & monitor) override;
void unlisten(epics::pvData::MonitorPtr const & monitor) override;

private:
std::string m_channel_name;
//epics::pvData::MonitorPtr monitor;
Monitor::wptr monitor_HL;
};

MonitorRequester::MonitorRequester(std::string channel_name, Monitor::wptr monitor_HL) :
	m_channel_name(channel_name),
	monitor_HL(monitor_HL)
{
}


MonitorRequester::~MonitorRequester() {
	LOG(7, "dtor");
}

string MonitorRequester::getRequesterName() { return "MonitorRequester"; }

void MonitorRequester::message(string const & msg, epics::pvData::MessageType msgT) {
	LOG(4, "MonitorRequester::message: {}", msg.c_str());
}


void MonitorRequester::monitorConnect(epics::pvData::Status const & status, epics::pvData::Monitor::shared_pointer const & monitor_, epics::pvData::StructureConstPtr const & structure) {
	auto monitor_HL = this->monitor_HL.lock();
	if (!status.isSuccess()) {
		// NOTE
		// Docs does not say anything about whether we are responsible for any handling of the monitor if non-null?
		LOG(1, "ERROR nosuccess");
		if (monitor_HL) {
			monitor_HL->go_into_failure_mode();
		}
	}
	else {
		if (status.isOK()) {
			LOG(7, "success and OK");
		}
		else {
			LOG(4, "success with warning:  [[TODO STATUS]]");
		}
	}
	//monitor = monitor_;
	monitor_->start();
}




void MonitorRequester::monitorEvent(epics::pvData::MonitorPtr const & monitor) {
	//LOG(7, "monitorEvent seq {}", seq);

	auto monitor_HL = this->monitor_HL.lock();
	if (!monitor_HL) {
		LOG(2, "monitor_HL already gone");
		return;
	}

	#if TEST_RANDOM_FAILURES
		std::mt19937 rnd(std::chrono::system_clock::now().time_since_epoch().count());
		if (rnd() < 0.12 * 0xffffffff) {
			monitor_HL->go_into_failure_mode();
		}
	#endif

	while (auto ele = monitor->poll()) {
		static_assert(sizeof(uint64_t) == sizeof(std::chrono::nanoseconds::rep), "Types not compatible");
		uint64_t ts = std::chrono::duration_cast<std::chrono::nanoseconds>(
			std::chrono::system_clock::now().time_since_epoch()
		).count();

		// Seems like MonitorElement always returns a Structure type ?
		// The inheritance diagram shows that scalars derive from Field, not from Structure.
		// Does that mean that we never get a scalar here directly??

		// TODO optimize a bit..
		auto & tms = monitor_HL->topic_mapping->topic_mapping_settings;
		FlatBufs::EpicsPVUpdate up;
		up.channel = tms.channel;
		up.pvstr = ele->pvStructurePtr;
		up.seq = seq;
		up.ts_epics_monitor = ts;
		up.fwdix = monitor_HL->forwarder_ix;
		up.teamid = tms.teamid;
		auto fb = tms.converter_epics_to_fb->convert(up);
		seq += 1;
		monitor_HL->emit(std::move(fb));
		monitor->release(ele);
	}
}

void MonitorRequester::unlisten(epics::pvData::MonitorPtr const & monitor) {
	LOG(4, "monitor source no longer available");
}









class StartMonitorChannel : public ActionOnChannel {
public:
StartMonitorChannel(Monitor::wptr monitor) : ActionOnChannel(monitor) { }

void operator () (epics::pvAccess::Channel::shared_pointer const & channel) override {
	auto monitor = this->monitor.lock();
	if (monitor) {
		monitor->initiate_value_monitoring();
	}
}

private:
epics::pvAccess::GetFieldRequester::shared_pointer gfr;
};








//                 Monitor

Monitor::Monitor(TopicMapping * topic_mapping, std::string channel_name, int forwarder_ix) :
	topic_mapping(topic_mapping),
	channel_name(channel_name),
	forwarder_ix(forwarder_ix)
{
	if (!topic_mapping) throw std::runtime_error("ERROR did not receive a topic_mapping");
}

void Monitor::init(std::shared_ptr<Monitor> self) {
	this->self = decltype(this->self)(self);
	initiate_connection();
}

Monitor::~Monitor() {
	LOG(7, "Monitor dtor");
	stop();
}

bool Monitor::ready() {
	return ready_monitor;
}

void Monitor::stop() {
	RMLG lg(m_mutex_emitter);
	try {
		if (mon) {
			//LOG(2, "stopping monitor for TM {}", topic_mapping->id);

			// TODO
			// After some debugging, it seems to me that even though we call stop() on Epics
			// monitor, the Epics library will continue to transfer data even though the monitor
			// will not produce events anymore.
			// This continues, until the monitor object is destructed.
			// Based on this observation, this code could use some refactoring.

			// Even though I have a smart pointer, calling stop() sometimes causes segfault within
			// Epics under load.  Not good.
			//mon->stop();

			// Docs say, that one must call destroy(), and it calims that 'delete' is prevented,
			// but that can't be true.
			mon->destroy();
			mon.reset();
		}
	}
	catch (std::runtime_error & e) {
		LOG(2, "Runtime error from Epics: {}", e.what());
		go_into_failure_mode();
	}
}


/**
A crutch to cope with Epics delayed release of resources.  Refactor...
*/
void Monitor::topic_mapping_gone() {
	RMLG lg(m_mutex_emitter);
	topic_mapping = nullptr;
}


void Monitor::go_into_failure_mode() {
	// Can be called from different threads, make sure we trigger only once.
	// Can also be called recursive, from different error conditions.

	// TODO
	// How to be sure that no callbacks are invoked any longer?
	// Does Epics itself provide a facility for that already?
	// Maybe not clear.  Epics invokes the channel state change sometimes really late
	// from the delayed deleter process.  (no documentation about this avail?)

	RMLG lg(m_mutex_emitter);

	if (failure_triggered.exchange(1) == 0) {
		stop();
		if (topic_mapping) {
			if (topic_mapping->forwarding) {
				topic_mapping->go_into_failure_mode();
			}
		}
	}
}


void Monitor::initiate_connection() {
	static bool do_init_factory_pva { true };
	//static bool do_init_factory_ca  { true };
	//static epics::pvAccess::ChannelProviderRegistry::shared_pointer provider;

	if (true) {
		if (do_init_factory_pva) {
			epics::pvAccess::ClientFactory::start();
			do_init_factory_pva = false;
		}
		provider = epics::pvAccess::getChannelProviderRegistry()
			->getProvider("pva");
	}
	/*
	else if (t == T::EPICS_CA_VALUE) {
		if (do_init_factory_ca) {
			epics::pvAccess::ca::CAClientFactory::start();
			do_init_factory_ca = true;
		}
		provider = epics::pvAccess::getChannelProviderRegistry()
			->getProvider("ca");
	}
	*/
	if (provider == nullptr) {
		LOG(4, "ERROR could not create a provider");
		throw epics_channel_failure();
	}
	//cr.reset(new ChannelRequester(std::move(new StartMonitorChannel())));
	cr.reset(new ChannelRequester(std::unique_ptr<StartMonitorChannel>(new StartMonitorChannel(self) )));
	ch = provider->createChannel(channel_name, cr);
}


void Monitor::initiate_value_monitoring() {
	RMLG lg(m_mutex_emitter);
	if (!topic_mapping) return;
	if (!topic_mapping->forwarding) return;
	monr.reset(new MonitorRequester(channel_name, self));

	// Leaving it empty seems to be the full channel, including name.  That's good.
	// Can also specify subfields, e.g. "value, timeStamp"  or also "field(value)"
	string request = "";
	PVStructure::shared_pointer pvreq;
	pvreq = epics::pvData::CreateRequest::create()->createRequest(request);
	mon = ch->createMonitor(monr, pvreq);
	if (!mon) {
		throw std::runtime_error("ERROR could not create EPICS monitor instance");
	}
	if (mon) {
		ready_monitor = true;
	}
}

void Monitor::emit(BrightnESS::FlatBufs::FB_uptr fb) {
	// TODO
	// It seems to be hard to tell when Epics won't use the callback anymore.
	// Instead of checking each access, use flags and quarantine before release
	// of memory.
	RMLG lg(m_mutex_emitter);
	if (!topic_mapping) return;
	if (!topic_mapping->forwarding) return;
	topic_mapping->emit(std::move(fb));
}


}
}
}
