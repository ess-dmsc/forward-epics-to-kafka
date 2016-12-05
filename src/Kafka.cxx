#include "Kafka.h"
#include "logger.h"
#include "local_config.h"

#include <map>
#include <algorithm>

// Kafka uses LOG_DEBUG as defined here:
#ifdef _MSC_VER
	#define LOG_DEBUG 7
#else
	#include <syslog.h>
#endif


namespace BrightnESS {
namespace ForwardEpicsToKafka {
namespace Kafka {

InstanceSet & InstanceSet::Set(std::string brokers, std::map<std::string, int> conf_ints) {
	LOG(3, "Kafka InstanceSet with rdkafka version: {}", rd_kafka_version_str());
	static std::unique_ptr<InstanceSet> kset;
	if (!kset) {
		kset.reset(new InstanceSet(brokers, conf_ints));
	}
	return *kset;
}

InstanceSet::InstanceSet(std::string brokers, std::map<std::string, int> conf_ints) : brokers(brokers), conf_ints(conf_ints) {
	for (int i1 = 0; i1 < KAFKA_INSTANCE_COUNT; ++i1) {
		instances.push_front(Instance::create(brokers, conf_ints));
	}
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
		instances.push_front(Instance::create(brokers, conf_ints));
		return instances.front();
		//throw std::runtime_error("error no instances available?");
	}
	return *it1;
}

int Instance::load() {
	return topics.size();
}


// Callbacks
// The callbacks can be set per Kafka instance, but not per topic.
// The message delivery callback can have a opaque specific to each message.

static void msg_delivered_cb(
	rd_kafka_t * rk,
	const rd_kafka_message_t * rkmessage,
	void * opaque
) {
	// NOTE the opaque here is the one given during produce.

	// TODO
	// Use callback to reuse our message buffers
	if (rkmessage->err) {
		LOG(6, "ERROR on delivery, topic {}, {}, {}",
			rd_kafka_topic_name(rkmessage->rkt),
			rd_kafka_err2str(rkmessage->err),
			rd_kafka_message_errstr(rkmessage));
	}
	else {
		//LOG(3, "OK delivered ({} bytes, offset {}, partition {}): {:.{}}\n",
		//	rkmessage->len, rkmessage->offset, rkmessage->partition,
		//	(const char *)rkmessage->payload, (int)rkmessage->len);
	}
	auto fb = static_cast<BrightnESS::FlatBufs::FB*>(rkmessage->_private);
	if (fb) {
		delete fb;
	}
}

static void kafka_error_cb(rd_kafka_t * rk, int err_i, const char * reason, void * opaque) {
	// cast necessary because of Kafka API design
	rd_kafka_resp_err_t err = (rd_kafka_resp_err_t) err_i;
	LOG(7, "ERROR Kafka: {}, {}, {}, {}", err_i, rd_kafka_err2name(err), rd_kafka_err2str(err), reason);

	// Can not throw, as it's Kafka's thread.
	// Must notify my watchdog though.
	auto ins = reinterpret_cast<KafkaOpaqueType*>(opaque);
	ins->error_from_kafka_callback();
}



static int stats_cb_kafka(rd_kafka_t * rk, char * json, size_t json_len, void * opaque) {
	LOG(3, "INFO stats_cb length {}   {:.{}}", json_len, json, json_len);
	// TODO
	// What does Kafka want us to return from this callback?
	return 0;
}



Instance::Instance() {
	static int id_ = 0;
	id = id_++;
	LOG(5, "Instance {} created.", id.load());
}

Instance::~Instance() {
	LOG(5, "Instance {} goes away.", id.load());
	poll_stop();
	if (rk) {
		LOG(3, "try to destroy kafka");
		rd_kafka_destroy(rk);
	}
}


sptr<Instance> Instance::create(std::string brokers, std::map<std::string, int> conf_ints) {
	auto ret = sptr<Instance>(new Instance);
	ret->brokers = brokers;
	ret->self = std::weak_ptr<Instance>(ret);
	ret->conf_ints = conf_ints;
	ret->init();
	return ret;
}






void kafka_log_cb(rd_kafka_t const * rk, int level, char const * fac, char const * buf) {
	LOG(level, "{}  fac: {}", buf, fac);
}



void Instance::init() {
	std::map<std::string, int> conf_ints {
		{"statistics.interval.ms",                   20 * 1000},
		{"metadata.request.timeout.ms",              15 * 1000},
		{"socket.timeout.ms",                         4 * 1000},
		{"session.timeout.ms",                       15 * 1000},

		{"message.max.bytes",                 23 * 1024 * 1024},
		//{"message.max.bytes",                       512 * 1024},

		// check again these two?
		{"fetch.message.max.bytes",            3 * 1024 * 1024},
		{"receive.message.max.bytes",          3 * 1024 * 1024},

		{"queue.buffering.max.messages",       2 * 1000 * 1000},
		//{"queue.buffering.max.kbytes",              800 * 1024},
		{"queue.buffering.max.ms",                        1000},

		// Total MessageSet size limited by message.max.bytes
		{"batch.num.messages",                       10 * 1000},
		{"socket.send.buffer.bytes",          23 * 1024 * 1024},
		{"socket.receive.buffer.bytes",       23 * 1024 * 1024},

		// Consumer
		//{"queued.min.messages", "1"},
	};

	for (auto & c : this->conf_ints) {
		conf_ints[c.first] = c.second;
	}

	// librdkafka API sometimes wants to write errors into a buffer:
	int const errstr_N = 512;
	char errstr[errstr_N];

	rd_kafka_conf_t * conf = 0;
	conf = rd_kafka_conf_new();
	rd_kafka_conf_set_dr_msg_cb(conf, msg_delivered_cb);
	rd_kafka_conf_set_error_cb(conf, kafka_error_cb);
	rd_kafka_conf_set_stats_cb(conf, stats_cb_kafka);

	// Let compiler check type first:
	KafkaOpaqueType * op1 = this;
	rd_kafka_conf_set_opaque(conf, op1);

	rd_kafka_conf_set_log_cb(conf, kafka_log_cb);

	for (auto & c : conf_ints) {
		LOG(7, "Set config: {} = {}", c.first.c_str(), c.second);
		if (RD_KAFKA_CONF_OK != rd_kafka_conf_set(conf, c.first.c_str(), fmt::format("{:d}", c.second).c_str(), errstr, errstr_N)) {
			LOG(7, "ERROR setting config: {}", c.first.c_str());
		}
	}

	rk = rd_kafka_new(RD_KAFKA_PRODUCER, conf, errstr, errstr_N);
	if (!rk) {
		LOG(7, "ERROR can not create kafka handle: {}", errstr);
		throw std::runtime_error("can not create Kafka handle");
	}

	LOG(3, "Name of the new Kafka handle: {}", rd_kafka_name(rk));

	rd_kafka_set_log_level(rk, LOG_DEBUG);

	LOG(3, "Brokers: {}", brokers.c_str());
	if (rd_kafka_brokers_add(rk, brokers.c_str()) == 0) {
		LOG(7, "ERROR could not add brokers");
		throw std::runtime_error("could not add brokers");
	}

	poll_start();
}


void Instance::poll_start() {
	LOG(0, "START polling");
	do_poll = true;
	// NOTE
	// All Kafka callbacks are also invoked from that thread:
	poll_thread = std::thread(&Instance::poll_run, this);
}

void Instance::poll_run() {
	int i1 = 0;
	while (do_poll) {
		int n1 = rd_kafka_poll(rk, 500);
		uint64_t ts = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
		LOG(0, "poll served callbacks ts {}  n1 {}  queue {}", ts, n1, rd_kafka_outq_len(rk));
		i1 += 1;
	}
	LOG(3, "Poll finished");
}

void Instance::poll_stop() {
	do_poll = false;
	poll_thread.join();
	LOG(3, "Poll thread joined");
}


void Instance::error_from_kafka_callback() {
	if (!error_from_kafka_callback_flag.exchange(true)) {
		for (auto & tmw : topics) {
			if (auto tm = tmw.lock()) {
				tm->failure = true;
			}
		}
	}
}


sptr<Topic> Instance::get_or_create_topic(std::string topic_name) {
	if (instance_failure()) {
		return nullptr;
	}
	// NOTE
	// Not thread safe.
	// But only called from the main setup thread.
	check_topic_health();
	for (auto & x : topics) {
		if (auto sp = x.lock()) {
			if (sp->topic_name() == topic_name) {
				LOG(3, "reuse topic \"{}\", using {} so far", topic_name.c_str(), topics.size());
				return sp;
			}
		}
	}
	auto ins = self.lock();
	if (!ins) {
		LOG(3, "ERROR self is no longer alive");
		return nullptr;
	}
	auto sp = sptr<Topic>(new Topic(ins, topic_name));
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
				LOG(9, "WEIRD, shouldnt that be expired?");
			}
			LOG(3, "No producer.  Already dtored?");
			return true;
		}
		if (t1.lock() == nullptr && !t1.expired()) {
			LOG(9, "ERROR weak ptr: no lock(), but not expired() either");
			return true;
		}
		return false;
	});
	topics.erase(it2, topics.end());
}



bool Instance::instance_failure() {
	return error_from_kafka_callback_flag;
}



int32_t partitioner_example(
	rd_kafka_topic_t const * rkt,
	void const * keydata,
	size_t keylen,
	int32_t partition_cnt,
	void * rkt_opaque,
	void * msg_opaque)
{
	// This callback is only allowed to call rd_kafka_topic_partition_available
	LOG(7, "WARNING WARNING WARNING WARNING partitioner_example NOT IMPLEMENTED");
	return RD_KAFKA_PARTITION_UA;
}




Topic::Topic(sptr<Instance> ins, std::string topic_name)
: ins(ins), topic_name_(topic_name)
{
	// librdkafka API sometimes wants to write errors into a buffer:
	int const errstr_N = 512;
	char errstr[errstr_N];

	rd_kafka_topic_conf_t * topic_conf = rd_kafka_topic_conf_new();
	{
		std::vector<std::vector<std::string>> confs = {
			{"produce.offset.report", "false"},
			{"request.required.acks", "0"},
			{"message.timeout.ms", "15000"},
		};
		for (auto & c : confs) {
			if (RD_KAFKA_CONF_OK != rd_kafka_topic_conf_set(topic_conf, c.at(0).c_str(), c.at(1).c_str(), errstr, errstr_N)) {
				LOG(7, "error setting config: {}", c.at(0).c_str());
			}
		}
	}

	//rd_kafka_topic_conf_set_partitioner_cb(topic_conf, rd_kafka_msg_partitioner_random);
	//rd_kafka_topic_conf_set_partitioner_cb(topic_conf, rd_kafka_msg_partitioner_consistent);
	//rd_kafka_topic_conf_set_partitioner_cb(topic_conf, rd_kafka_msg_partitioner_consistent_random);
	rd_kafka_topic_conf_set_partitioner_cb(topic_conf, partitioner_example);

	rkt = rd_kafka_topic_new(ins->rk, topic_name.c_str(), topic_conf);
	if (rkt == nullptr) {
		// Seems like Kafka uses the system error code?
		auto errstr = rd_kafka_err2str(rd_kafka_errno2err(errno));
		LOG(7, "ERROR could not create Kafka topic: {}", errstr);
		throw std::exception();
	}
	LOG(0, "OK, seems like we've added topic {}", rd_kafka_topic_name(rkt));
}

Topic::~Topic() {
	if (rkt) {
		LOG(0, "destroy topic");
		rd_kafka_topic_destroy(rkt);
		rkt = nullptr;
	}
}



void Topic::produce(BrightnESS::FlatBufs::FB_uptr fb, uint64_t seq, uint64_t ts) {
	int x;
	int32_t partition = RD_KAFKA_PARTITION_UA;
	partition = seq % 5;

	// Optional:
	void const * key = NULL;
	size_t key_len = 0;

	void * callback_data = fb.get();
	// no flags means that we reown our buffer when Kafka calls our callback.
	int msgflags = 0; // 0, RD_KAFKA_MSG_F_COPY, RD_KAFKA_MSG_F_FREE

	// TODO
	// How does Kafka report the error?
	// API docs state that error codes are given in 'errno'
	// Check that this is thread safe ?!?

	auto m1 = fb->message();
	LOG(0, "produce seq {}  ts {}  len {}", seq, ts, m1.size);

	//x = rd_kafka_produce(rkt, partition, msgflags, buf.begin, buf.size, key, key_len, callback_data);
	x = rd_kafka_produce(rkt, partition, msgflags, m1.data, m1.size, key, key_len, callback_data);
	if (x == RD_KAFKA_RESP_ERR__QUEUE_FULL) {
		LOG(7, "ERROR OutQ: {}  QUEUE_FULL  Dropping message seq {}", rd_kafka_outq_len(ins->rk), seq);
		return;
	}
	if (x == RD_KAFKA_RESP_ERR_MSG_SIZE_TOO_LARGE) {
		LOG(7, "ERROR OutQ: {}  TOO_LARGE seq {}", rd_kafka_outq_len(ins->rk), seq);
		return;
	}
	if (x != 0) {
		LOG(7, "ERROR on produce topic {}  partition {}  seq {}: {}",
			rd_kafka_topic_name(rkt),
			partition,
			seq,
			rd_kafka_err2str(rd_kafka_last_error())
		);
		//throw std::runtime_error("ERROR on message send");
		// even when taking out exception in future, return here
		return;
	}

	// After no Kafka error was reported on produce, we borrow the object to Kafka
	// until the callback is called.
	fb.release();

	//LOG(0, "sent to topic {} partition {}", rd_kafka_topic_name(rkt), partition);
}




std::string & Topic::topic_name() { return topic_name_; }

bool Topic::healthy() {
	return !failure;
}





}
}
}
