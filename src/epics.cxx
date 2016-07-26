/** \file
Implements the requester classes that Epics uses as callbacks.
Be careful with exceptions as the Epics C++ API does not clearly state
from which threads the callbacks are invoked.
Instead, we call the supervising class which in turn stops operation
and marks the mapping as failed.  Cleanup is then triggered in the main
watchdog thread.
*/

#include "epics.h"
#include "logger.h"
#include "TopicMapping.h"
#include <exception>
#include <random>
#include "config.h"


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


char const * field_type_name(epics::pvData::Type x) {
#define DWTN1(N) DWTN2(N, STRINGIFY(N))
#define DWTN2(N, S) if (x == epics::pvData::N) { return S; }
	DWTN1(scalar);
	DWTN1(scalarArray);
	DWTN1(structure);
	DWTN1(structureArray);
	DWTN2(union_, "union");
	DWTN1(unionArray);
#undef DWTN1
#undef DWTN2
	return "[unknown]";
}

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



void inspect_Structure(Structure const & str, int level = 0);


void inspect_Field(epics::pvData::Field const & field, int level = 0) {
	using namespace epics::pvData;
	LOG(0, "getID(): %s   getType(): %s", field.getID().c_str(), field_type_name(field.getType()));
	if (auto p1 = dynamic_cast<Structure const *>(&field)) {
		inspect_Structure(*p1, level);
	}
	else {
		LOG(0, "NO-SPECIAL-CAST");
	}
}


void inspect_Structure(epics::pvData::Structure const & str, int level) {
	LOG(0, "getNumberFields: %u", str.getNumberFields());
	auto & subfields = str.getFields();
	LOG(0, "n subfields: %u", subfields.size());
	int i1 = 0;
	for (auto & f1sptr : subfields) {
		auto & f1 = *f1sptr.get();
		LOG(0, "%*snow inspect subfield [%s]", 2*level, "", str.getFieldName(i1).c_str());
		inspect_Field(f1, 1+level);
		++i1;
	}
}




void inspect_PVStructure(epics::pvData::PVStructure const & pvstr, int level = 0);


void inspect_PVField(epics::pvData::PVField const & field, int level = 0) {
	// TODO
	// After this initial 'getting-to-know-Epics' refactor into polymorphic access
	LOG(0, "%*sgetFieldName: %s", 2*level, "", field.getFieldName().c_str());
	LOG(0, "getNumberFields: %u", field.getNumberFields());
	if (auto p1 = dynamic_cast<PVStructure const *>(&field)) {
		inspect_PVStructure(*p1, level);
	}
	else if (auto p1 = dynamic_cast<epics::pvData::PVScalarValue<double> const *>(&field)) {
		LOG(0, "[double == % e]", p1->get());
	}
	else {
		LOG(0, "No special PV-Cast");
	}
}




void inspect_PVStructure(epics::pvData::PVStructure const & pvstr, int level) {
	// getStructure is only on PVStructure, not on PVField
	//auto & str = *pvstr.getStructure();

	auto & subfields = pvstr.getPVFields();
	LOG(0, "subfields.size(): %u", subfields.size());
	for (auto & f1ptr : subfields) {
		auto & f1 = *f1ptr;
		inspect_PVField(f1, 1+level);
	}
}




class ActionOnChannel {
public:
ActionOnChannel(Monitor & monitor) : monitor(monitor) { }
virtual void operator () (epics::pvAccess::Channel::shared_pointer const & channel) {
	LOG(5, "[EMPTY ACTION]");
};
Monitor & monitor;
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
	LOG(3, "Message for: %s  msg: %s  msgtype: %s", getRequesterName().c_str(), message.c_str(), getMessageTypeName(messageType).c_str());
}



/*
Seems that channel creation is actually a synchronous operation
and that this requester callback is called from the same stack
from which the channel creation was initiated.
*/

void ChannelRequester::channelCreated(epics::pvData::Status const & status, Channel::shared_pointer const & channel) {
	#if TEST_RANDOM_FAILURES
		std::mt19937 rnd(std::chrono::system_clock::now().time_since_epoch().count());
		if (rnd() < 0.1 * 0xffffffff) {
			action->monitor.go_into_failure_mode();
		}
	#endif
	if (!status.isSuccess()) {
		// quick fix until decided on logging system..
		std::ostringstream s1;
		s1 << status;
		LOG(6, "failure: %s", s1.str().c_str());
		if (channel) {
			// Yes, take a copy
			std::string cname = channel->getChannelName();
			LOG(6, "  failure is in channel: %s", cname.c_str());
		}
		action->monitor.go_into_failure_mode();
	}
	LOG(0, "success (%d)", (int)status.isOK());
	if (!status.isOK()) {
		// quick fix until decided on logging system..
		std::ostringstream s1;
		s1 << status;
		LOG(3, "WARNING %s", s1.str().c_str());
	}
}

void ChannelRequester::channelStateChange(Channel::shared_pointer const & channel, Channel::ConnectionState cstate) {
	LOG(0, "channel state change: %s", Channel::ConnectionStateNames[cstate]);
	if (cstate == Channel::DISCONNECTED) {
		LOG(1, "Epics channel disconnect");
		action->monitor.go_into_failure_mode();
		return;
	}
	else if (cstate == Channel::DESTROYED) {
		//throw epics_channel_failure();
		LOG(1, "Epics channel destroyed");
		action->monitor.go_into_failure_mode();
		return;
	}
	else if (cstate != Channel::CONNECTED) {
		LOG(6, "Unhandled channel state change: %s", channel_state_name(cstate));
		action->monitor.go_into_failure_mode();
	}
	if (!channel) {
		LOG(6, "ERROR no channel, even though we should have.  state: %s", channel_state_name(cstate));
		action->monitor.go_into_failure_mode();
	}

	action->operator()(channel);

		#if 0
		if (true) {
			// Initiate a single channel get

			// TODO why not just pass the channel if we have it anyway?
			// And what do we use ChannelGetRequesterDW for anyway???
			cgr.reset(new ChannelGetRequesterDW());
			//ChannelGetRequesterDW * cgri = dynamic_cast<ChannelGetRequesterDW*>(cgr.get());

			// Is this some magic string?!?!??
			// Yes.  Not yet sure about how it works.
			// If I specify 'value' I get the same as with an empty string.
			// A null errors.
			// 'alarm' gives me the alarm structure, and my inspect routine sees it.
			// But how do I inspect everything in that channel if I do not know that e.g. 'alarm' exists???

			// The channel->getField() can do that, see above.  That will return the full
			// introspection data and I can see all the primary fields of the channel.

			// Is there no way to do a wildcard request at this point ??

			string request = "value,timeStamp";
			PVStructure::shared_pointer pvreq;
			pvreq = epics::pvData::CreateRequest::create()->createRequest(request);
			if (pvreq == nullptr) {
				LOG0("can not create a pv request");
				throw exception();
			}

			// Must keep the returned channel get instance alive
			cg = channel->createChannelGet(cgr, pvreq);
		}


		if (false) {
			// Initiate a monitor on the channel
			monr.reset(new MonitorRequester);
			string request = "";
			PVStructure::shared_pointer pvreq;
			pvreq = epics::pvData::CreateRequest::create()->createRequest(request);
			mon = channel->createMonitor(monr, pvreq);
		}
		#endif

}




class ActionOnField {
public:
ActionOnField(Monitor & monitor) : monitor(monitor) { }
virtual void operator () (Field const & field) { };
Monitor & monitor;
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
	LOG(0, STRINGIFY(GetFieldRequesterForAction) " ctor");
}

string GetFieldRequesterForAction::getRequesterName() { return STRINGIFY(GetFieldRequesterForAction); }

void GetFieldRequesterForAction::message(string const & msg, epics::pvData::MessageType msgT) {
	LOG(3, "%s", msg.c_str());
}

void GetFieldRequesterForAction::getDone(epics::pvData::Status const & status, epics::pvData::FieldConstPtr const & field) {
	if (!status.isSuccess()) {
		LOG(6, "ERROR nosuccess");
		action->monitor.go_into_failure_mode();
	}
	if (status.isOK()) {
		LOG(0, "success and OK");
	}
	else {
		LOG(3, "success with warning:  [[TODO STATUS]]");
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
MonitorRequester(Monitor & monitor_HL);
~MonitorRequester();
string getRequesterName() override;
void message(string const & msg, epics::pvData::MessageType msgT) override;

void monitorConnect(epics::pvData::Status const & status, epics::pvData::Monitor::shared_pointer const & monitor, epics::pvData::StructureConstPtr const & structure) override;

void monitorEvent(epics::pvData::MonitorPtr const & monitor) override;
void unlisten(epics::pvData::MonitorPtr const & monitor) override;

private:
epics::pvData::MonitorPtr monitor;
Monitor & monitor_HL;
};

MonitorRequester::MonitorRequester(Monitor & monitor_HL) :
	monitor_HL(monitor_HL)
{
}


MonitorRequester::~MonitorRequester() {
	LOG(0, "dtor");
}

string MonitorRequester::getRequesterName() { return "MonitorRequester"; }

void MonitorRequester::message(string const & msg, epics::pvData::MessageType msgT) {
	LOG(3, "%s", msg.c_str());
}


void MonitorRequester::monitorConnect(epics::pvData::Status const & status, epics::pvData::Monitor::shared_pointer const & monitor_, epics::pvData::StructureConstPtr const & structure) {
	if (!status.isSuccess()) {
		// NOTE
		// Docs does not say anything about whether we are responsible for any handling of the monitor if non-null?
		LOG(6, "ERROR nosuccess");
		monitor_HL.go_into_failure_mode();
	}
	else {
		if (status.isOK()) {
			LOG(0, "success and OK");
		}
		else {
			LOG(3, "success with warning:  [[TODO STATUS]]");
		}
	}
	monitor = monitor_;
	monitor->start();
}



// And again, the docs are basically non-existent.
// But according to the example, we must release the 'element' in the end.
// TODO
// Is it possible to miss events if we poll() only once per 'monitorEvent' ?

void MonitorRequester::monitorEvent(epics::pvData::MonitorPtr const & monitor) {
	LOG(0, "got event");

	#if TEST_RANDOM_FAILURES
		std::mt19937 rnd(std::chrono::system_clock::now().time_since_epoch().count());
		if (rnd() < 0.12 * 0xffffffff) {
			monitor_HL.go_into_failure_mode();
		}
	#endif

	// Docs for MonitorRequester says that we have to call poll()
	// This seems weird naming to me, because I listen to the Monitor because I do not want to poll?!?
	while (auto ele = monitor->poll()) {

		// Seems like MonitorElement always returns a Structure type ?
		// The inheritance diagram shows that scalars derive from Field, not from Structure.
		// Does that mean that we never get a scalar here directly??

		auto & pvstr = ele->pvStructurePtr;

		//inspect_PVField(*pvstr);

		// A PVStructure is-a PVField
		//LOG0("getFieldName(): %s", pvstr->getFieldName().c_str());
		//LOG0("getFullName():  %s", pvstr->getFullName().c_str());

		if (false) {
			// One possibility:  get the Field
			auto & field = pvstr->getField();
			LOG(0, "%lx", ele->pvStructurePtr->getField().get());
			LOG(0, "ele->pvStructurePtr->getField()->getID(): %s", ele->pvStructurePtr->getField()->getID().c_str());
		}

		if (false) {
			// Second possibility, get the 'introspection interface'
			// What are the differences??

			// A Structure derives from Field

			auto const & str = pvstr->getStructure();
			LOG(0, "%lx", ele->pvStructurePtr->getStructure().get());
			LOG(0, "getID(): %s", str->getID().c_str());
			LOG(0, "getNumberFields: %lu", str->getNumberFields());
			for (size_t i1 = 0; i1 < str->getNumberFields(); ++i1) {
				auto f1 = str->getField(i1);
				LOG(0, "Subfield [%s]  ID [%s] Type [%u]", str->getFieldName(i1).c_str(), f1->getID().c_str(), f1->getType());
			}
		}

		// TODO
		// This is hardcoded, test-code:
		// It assumes that we request the 'value' part.
		if (auto sf1 = pvstr->getSubField("value")) {
			if (auto p1 = dynamic_cast<epics::pvData::PVScalarValue<double> const *>(sf1.get())) {
				monitor_HL.emit(p1->get());
			}
		}
		monitor->release(ele);
	}
}

void MonitorRequester::unlisten(epics::pvData::MonitorPtr const & monitor) {
	LOG(3, "monitor source no longer available");
}









class IntrospectField : public ActionOnField {
public:
IntrospectField(Monitor & monitor) : ActionOnField(monitor) { }
void operator () (Field const & field) override {
	LOG(0, "inspecting Field disabled...");
	// TODO
	// Check that we understand the field content.
	// Need to discuss possible constraints?
	//inspect_Field(field);

	monitor.initiate_value_monitoring();
}
};








class StartMonitorChannel : public ActionOnChannel {
public:
StartMonitorChannel(Monitor & monitor) : ActionOnChannel(monitor) { }

void operator () (epics::pvAccess::Channel::shared_pointer const & channel) override {
	// Channel::getField is used to get the introspection information.
	// It is not necessary if we are only interested in the value itself.
	gfr.reset(new GetFieldRequesterForAction(channel, std::unique_ptr<IntrospectField>(new IntrospectField(monitor) )));

	// 2nd parameter:
	// http://epics-pvdata.sourceforge.net/docbuild/pvAccessCPP/tip/documentation/html/classepics_1_1pvAccess_1_1Channel.html#a506309575eed705e97cf686bd04f5c04

	// 2nd argument does maybe specify the subfield, not tested yet.
	// 'pvinfo' actually uses an empty string here.
	// Is empty string then 'the whole channel itself' ?  Apparently yes.
	// TODO try also NULL.
	channel->getField(gfr, "");
}

private:
epics::pvAccess::GetFieldRequester::shared_pointer gfr;
};








//                 Monitor

Monitor::Monitor(TopicMapping & topic_mapping, std::string channel_name) :
	topic_mapping(topic_mapping),
	channel_name(channel_name)
{
	mon.reset();
	if (!&topic_mapping) throw std::runtime_error("HMMMM");
	initiate_connection();
}

Monitor::~Monitor() {
	LOG(0, "Monitor dtor");
	stop();
}

bool Monitor::ready() {
	return ready_monitor;
}

void Monitor::stop() {
	if (mon) mon->stop();
}


void Monitor::go_into_failure_mode() {
	// Can be called from different threads, make sure we trigger only once.
	// TODO
	// How to be sure that no callbacks are invoked any longer?
	// Does Epics itself provide a facility for that already?

	// TODO
	// We should never come here with ~TopicMapping on the stack.
	// check again..

	if (failure_triggered.exchange(1) == 0) {
		LOG(0, "failure mode for topic mapping %d", topic_mapping.id);
		stop();
		topic_mapping.go_into_failure_mode();
	}
}


void Monitor::initiate_connection() {
	// TODO
	// Does it do any harm to call this multiple times per thread/process?
	epics::pvAccess::ClientFactory::start();
	provider = epics::pvAccess::getChannelProviderRegistry()
		->getProvider("pva");
	if (provider == nullptr) {
		LOG(3, "ERROR could not create a provider");
		throw epics_channel_failure();
	}
	//cr.reset(new ChannelRequester(std::move(new StartMonitorChannel())));
	cr.reset(new ChannelRequester(std::unique_ptr<StartMonitorChannel>(new StartMonitorChannel(*this) )));
	ch = provider->createChannel(channel_name, cr);
}


void Monitor::initiate_value_monitoring() {
	monr.reset(new MonitorRequester(*this));

	// Leaving it empty apparently is the same as 'value'
	string request = "value";
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

void Monitor::emit(double x) {
	LOG(0, "emitting % e", x);
	topic_mapping.emit(x);
}






}
}
}
