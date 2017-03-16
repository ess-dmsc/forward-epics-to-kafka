#include "Kafka.h"
#include "logger.h"
#include "local_config.h"

#include <map>
#include <algorithm>
#include <functional>

#ifdef _MSC_VER
	#define TLOG(level, fmt, ...) { \
		LOG(level, "TMID: {}  " fmt, this->id, __VA_ARGS__); \
	}
#else
	#define TLOG(level, fmt, args...) { \
		LOG(level, "TMID: {}  " fmt, this->id, ## args); \
	}
#endif

#ifdef _MSC_VER
	#define ILOG(level, fmt, ...) { \
		LOG(level, "IID: {}  " fmt, this->id, __VA_ARGS__); \
	}
#else
	#define ILOG(level, fmt, args...) { \
		LOG(level, "IID: {}  " fmt, this->id, ## args); \
	}
#endif

namespace BrightnESS {
namespace ForwardEpicsToKafka {
namespace Kafka {

InstanceSet & InstanceSet::Set(KafkaW::BrokerOpt opt) {
	LOG(4, "Kafka InstanceSet with rdkafka version: {}", rd_kafka_version_str());
	static std::unique_ptr<InstanceSet> kset;
	if (!kset) {
		kset.reset(new InstanceSet(opt));
	}
	return *kset;
}

InstanceSet::InstanceSet(KafkaW::BrokerOpt opt) : opt(opt) {
	instances.push_front(Instance::create(opt));
}

/**
Find and return the instance with the lowest load.
Load is currently defined as topic count, even though that might not reflect the actual load.
*/
sptr<Instance> InstanceSet::instance() {
	auto it1 = instances.end();
	auto LIM = std::numeric_limits<size_t>::max();
	size_t min = LIM;
	for (auto it2 = instances.begin(); it2 != instances.end(); ++it2) {
		if (!(*it2)->instance_failure()) {
			if ((*it2)->topics.size() < min  ||  min == LIM) {
				min = (*it2)->topics.size();
				it1 = it2;
			}
		}
	}
	if (it1 == instances.end()) {
		// TODO
		// Need to throttle the creation of instances in case of permanent failure
		instances.push_front(Instance::create(opt));
		return instances.front();
		//throw std::runtime_error("error no instances available?");
	}
	return *it1;
}

int Instance::load() {
	return topics.size();
}


struct Instance_impl {
Instance_impl();
~Instance_impl();
std::function<void(rd_kafka_message_t const * msg)> on_delivery_ok;
std::function<void(rd_kafka_message_t const * msg)> on_delivery_failed;
};

Instance_impl::Instance_impl() {
	on_delivery_ok = [] (rd_kafka_message_t const * msg) {
		BrightnESS::FlatBufs::FB_uptr p1((BrightnESS::FlatBufs::FB *)msg->_private);
	};
	on_delivery_failed = [] (rd_kafka_message_t const * msg) {
		BrightnESS::FlatBufs::FB_uptr p1((BrightnESS::FlatBufs::FB *)msg->_private);
	};
}

Instance_impl::~Instance_impl() {
}


Instance::Instance(KafkaW::BrokerOpt opt) : opt(opt), producer(KafkaW::Producer(opt)) {
	impl.reset(new Instance_impl);
	static int id_ = 0;
	id = id_++;
	LOG(4, "Instance {} created.", id.load());
}

Instance::~Instance() {
	LOG(4, "Instance {} goes away.", id.load());
	poll_stop();
}


sptr<Instance> Instance::create(KafkaW::BrokerOpt opt) {
	auto ret = sptr<Instance>(new Instance(opt));
	ret->self = std::weak_ptr<Instance>(ret);
	ret->init();
	return ret;
}




void Instance::init() {
	producer.on_delivery_ok = impl->on_delivery_ok;
	producer.on_delivery_failed = impl->on_delivery_failed;
	poll_start();
}


void Instance::poll_start() {
	ILOG(7, "START polling");
	do_poll = true;
	poll_thread = std::thread(&Instance::poll_run, this);
}

void Instance::poll_run() {
	int i1 = 0;
	while (do_poll) {
		producer.poll();
		i1 += 1;
		std::this_thread::sleep_for(std::chrono::milliseconds(750));
	}
	ILOG(7, "Poll finished");
}

void Instance::poll_stop() {
	do_poll = false;
	poll_thread.join();
	ILOG(7, "Poll thread joined");
}


void Instance::error_from_kafka_callback() {

	// TODO
	// Need to set a callback on the kafka producer which knows about Instance.
	// In order to avoid passing opaques, should use a lambda as callback and std::function.

	if (!error_from_kafka_callback_flag.exchange(true)) {
		for (auto & tmw : topics) {
			if (auto tm = tmw.lock()) {
				tm->failure = true;
			}
		}
	}
}


sptr<Topic> Instance::get_or_create_topic(std::string topic_name, int id) {
	if (instance_failure()) {
		return nullptr;
	}
	// NOTE
	// Not thread safe.
	// But only called from the main setup thread.
	check_topic_health();
	if (false) {
		for (auto & x : topics) {
			if (auto sp = x.lock()) {
				if (sp->topic_name() == topic_name) {
					LOG(4, "reuse topic \"{}\", using {} so far", topic_name.c_str(), topics.size());
					return sp;
				}
			}
		}
	}
	auto ins = self.lock();
	if (!ins) {
		LOG(3, "ERROR self is no longer alive");
		return nullptr;
	}
	auto sp = sptr<Topic>(new Topic(ins, topic_name, id));
	topics.push_back(std::weak_ptr<Topic>(sp));
	return sp;
}




/**
Check if the pointers to the topics are still valid, removes the expired ones.
Periodically called only from the main watchdog.
*/

void Instance::check_topic_health() {
	auto it2 = std::remove_if(topics.begin(), topics.end(), [](std::weak_ptr<Topic> const & t1){
		// TODO
		// Need to relate somehow the errors to a topic, or?
		// For the errors from the message callback it is possible.
		auto top = t1.lock();
		if (!top) {
			// Expired pointer should be the only reason why we do not get a lock
			if (!t1.expired()) {
				LOG(0, "WEIRD, shouldnt that be expired?");
			}
			LOG(4, "No producer.  Already dtored?");
			return true;
		}
		if (t1.lock() == nullptr && !t1.expired()) {
			LOG(0, "ERROR weak ptr: no lock(), but not expired() either");
			return true;
		}
		return false;
	});
	topics.erase(it2, topics.end());
}



bool Instance::instance_failure() {
	return error_from_kafka_callback_flag;
}



Topic::Topic(sptr<Instance> ins, std::string topic_name, int id)
	: ins(ins),
		topic_name_(topic_name),
		id(id),
		topic(KafkaW::Producer::Topic(ins->producer, topic_name))
{
}

Topic::~Topic() {
}



void Topic::produce(BrightnESS::FlatBufs::FB_uptr fb) {
	void * opaque = fb.get();
	auto m1 = fb->message();
	//LOG(7, "produce seq {}  ts {}  len {}", seq, ts, m1.size);

	// TODO
	// Change KafkaW to return errors and check what is to be done with fb.
	int x = topic.produce(m1.data, m1.size, opaque);

	auto rkt = topic.rkt;

	if (x == RD_KAFKA_RESP_ERR_NO_ERROR) {
		LOG(7, "sent seq {} to topic {} partition ??", fb->seq, rd_kafka_topic_name(rkt));
		fb.release();
		return;
	}

	if (x == RD_KAFKA_RESP_ERR__QUEUE_FULL) {
		TLOG(0, "ERROR OutQ: {}  QUEUE_FULL  Dropping message seq {}", rd_kafka_outq_len(ins->producer.rk), fb->seq);
	}
	if (x == RD_KAFKA_RESP_ERR_MSG_SIZE_TOO_LARGE) {
		TLOG(0, "ERROR OutQ: {}  TOO_LARGE seq {}", rd_kafka_outq_len(ins->producer.rk), fb->seq);
	}
	if (x != 0) {
		TLOG(0, "ERROR on produce topic {}  partition ??  seq {}: {}",
			rd_kafka_topic_name(rkt),
			fb->seq,
			rd_kafka_err2str(rd_kafka_last_error())
		);
	}
}




std::string & Topic::topic_name() { return topic_name_; }

bool Topic::healthy() {
	return !failure;
}





}
}
}
