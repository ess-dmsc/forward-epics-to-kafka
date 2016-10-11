#include "Config.h"
#include "logger.h"

#include <configuration.hpp>
#include <redox.hpp>


#ifdef _MSC_VER
	#define LOG_DEBUG 7
#else
	#include <syslog.h>
#endif

namespace BrightnESS {
namespace ForwardEpicsToKafka {
namespace Config {


// Uses the new lib
Service::Service() {
	using CM = configuration::communicator::RedisCommunicator;
	using DM = configuration::data::RedisDataManager<CM>;
	using Configuration = configuration::ConfigurationManager<DM,CM>;
	Configuration cs("localhost", 6379, "localhost", 6379);
	cs.Update("some_test", "garbage-value-here");
}





KafkaSettings::KafkaSettings(std::string brokers, std::string topic)
:	brokers(brokers),
	topic(topic)
{ }



static void kafka_error_cb(rd_kafka_t * rk, int err_i, const char * reason, void * opaque) {
	// cast necessary because of Kafka API design
	rd_kafka_resp_err_t err = (rd_kafka_resp_err_t) err_i;
	LOG(7, "ERROR Kafka Config: %d, %s, %s, %s", err_i, rd_kafka_err2name(err), rd_kafka_err2str(err), reason);
}



static void rebalance_cb(
	rd_kafka_t *rk,
	rd_kafka_resp_err_t err,
	rd_kafka_topic_partition_list_t *partitions,
	void *opaque)
{
	exit(1);
	LOG(9, "Consumer group rebalanced:");
	switch (err) {
	case RD_KAFKA_RESP_ERR__ASSIGN_PARTITIONS:
		LOG(3, "assigned:");
		//print_partition_list(stderr, partitions);
		rd_kafka_assign(rk, partitions);
		//wait_eof += partitions->cnt;
		break;
	case RD_KAFKA_RESP_ERR__REVOKE_PARTITIONS:
		LOG(3, "revoked:");
		//print_partition_list(stderr, partitions);
		rd_kafka_assign(rk, NULL);
		//wait_eof = 0;
		break;
	default:
		LOG(3, "failed: %s", rd_kafka_err2str(err));
		rd_kafka_assign(rk, NULL);
		break;
	}
}




#define KERR(err) if (err != 0) { LOG(1, "Kafka error code: %d", err); }



Listener::Listener(KafkaSettings settings) {
	static_assert(0 == RD_KAFKA_RESP_ERR_NO_ERROR, "0 == RD_KAFKA_RESP_ERR_NO_ERROR");

	int err;
	// librdkafka API sometimes wants to write errors into a buffer:
	int const errstr_N = 512;
	char errstr[errstr_N];

	auto conf = rd_kafka_conf_new();

	std::vector<std::vector<std::string>> confs = {
		{"message.max.bytes", "100000"},
		{"fetch.message.max.bytes", "100000"},
		//{"statistics.interval.ms", "10000"},
		{"metadata.request.timeout.ms", "2000"},
		{"socket.timeout.ms", "2000"},
		{"session.timeout.ms", "2000"},
		{"metadata.request.timeout.ms", "3000"},
		{"group.id", "configuration_global_consumer_group"},
	};
	for (auto & c : confs) {
		if (RD_KAFKA_CONF_OK != rd_kafka_conf_set(conf, c.at(0).c_str(), c.at(1).c_str(), errstr, errstr_N)) {
			LOG(7, "error setting config: %s", c.at(0).c_str());
		}
	}

	auto topic_conf = rd_kafka_topic_conf_new();
	{
		std::vector<std::vector<std::string>> confs = {
			{"message.timeout.ms", "2000"},
		};
		for (auto & c : confs) {
			if (RD_KAFKA_CONF_OK != rd_kafka_topic_conf_set(topic_conf, c.at(0).c_str(), c.at(1).c_str(), errstr, errstr_N)) {
				LOG(7, "error setting topic config: %s", c.at(0).c_str());
			}
		}
	}

	// To set a default configuration for regex matched topics:
	rd_kafka_conf_set_default_topic_conf(conf, topic_conf);

	//rd_kafka_conf_set_dr_msg_cb(conf, msg_delivered_cb);
	rd_kafka_conf_set_error_cb(conf, kafka_error_cb);
	//rd_kafka_conf_set_stats_cb(conf, stats_cb);
	rd_kafka_conf_set_rebalance_cb(conf, rebalance_cb);
	//rd_kafka_conf_set_opaque(conf, this);

	rk = rd_kafka_new(RD_KAFKA_CONSUMER, conf, errstr, errstr_N);
	if (!rk) {
		LOG(7, "ERROR can not create kafka handle: %s", errstr);
		throw std::runtime_error("can not create Kafka handle");
	}

	LOG(3, "Name of the new Kafka handle: %s", rd_kafka_name(rk));

	rd_kafka_set_log_level(rk, LOG_DEBUG);

	LOG(3, "Brokers: %s", settings.brokers.c_str());
	if (rd_kafka_brokers_add(rk, settings.brokers.c_str()) == 0) {
		LOG(7, "ERROR could not add brokers");
		throw std::runtime_error("could not add brokers");
	}

	rd_kafka_poll_set_consumer(rk);

	// TODO
	// When exactly to use _subscribe or _assign ?
	// Currently, it listens on partition 0, but when dumping the list of current subscriptions
	// in the poll(), it's empty.  Must be better way.
	// Currently, setting partition to -1 (all) does not work!

	// Listen to all partitions
	int partition = RD_KAFKA_PARTITION_UA;
	partition = 0;

	plist = rd_kafka_topic_partition_list_new(32);

	LOG(3, "Adding topic: %s", settings.topic.c_str());
	rd_kafka_topic_partition_list_add(plist, settings.topic.c_str(), partition);

	//err = rd_kafka_subscribe(rk, plist);
	err = rd_kafka_assign(rk, plist);
	KERR(err);
	if (err) {
		LOG(6, "ERROR could not subscribe");
		throw std::runtime_error("can not subscribe");
	}
}


Listener::~Listener() {
	if (plist) {
		rd_kafka_topic_partition_list_destroy(plist);
		plist = nullptr;
	}
	if (rk) {
		// commit offsets
		rd_kafka_consumer_close(rk);
		rd_kafka_destroy(rk);
		rk = nullptr;
	}
}




void Listener::kafka_connection_information() {
	if (true) {
		// Dump current subscription:
		rd_kafka_topic_partition_list_t * l1 = 0;
		rd_kafka_subscription(rk, &l1);
		if (l1) {
			for (int i1 = 0; i1 < l1->cnt; ++i1) {
				LOG(1, "subscribed topics: %s  %s  off %d", l1->elems[i1].topic, rd_kafka_err2str(l1->elems[i1].err), l1->elems[i1].offset);
			}
			rd_kafka_topic_partition_list_destroy(l1);
		}
	}

	if (true) {
		rd_kafka_dump(stdout, rk);
	}

	if (true) {
		// only do this if not redirected
		int n1 = rd_kafka_poll(rk, 0);
		LOG(1, "config list poll served %d events", n1);
	}
}


void Listener::poll(Callback & cb) {
	// Arbitrary limit configuration messages processed in one go:
	for (int i1 = 0; i1 < 10; ++i1) {
		rd_kafka_message_t * msg;
		msg = rd_kafka_consumer_poll(rk, 0);
		if (msg) {
			if (msg->err == RD_KAFKA_RESP_ERR_NO_ERROR) {
				//LOG(1, "msg");
				cb({(char*)msg->payload, msg->len});
				rd_kafka_message_destroy(msg);
			}
			else if (msg->err == RD_KAFKA_RESP_ERR__PARTITION_EOF) {
				// just an advisory
				// TODO
				// can we find out which partition it is?  I hope so..
			}
			else if (msg->err == RD_KAFKA_RESP_ERR__ALL_BROKERS_DOWN) {
				// TODO Change state and exit the poll loop
				// TODO Does librdkafka reconnect automatically?
				//   apparently not.  Clean up, try reconnect later.
				break;
			}
			else if (msg->err == RD_KAFKA_RESP_ERR__BAD_MSG) {
				LOG(3, "ERROR bad message");
			}
			else if (msg->err == RD_KAFKA_RESP_ERR__DESTROY) {
				// Broker will go away soon
				LOG(3, "WARNING broker will go away");
			}
			else {
				LOG(1, "ERROR unhandled msg error: %s %s", rd_kafka_err2name(msg->err), rd_kafka_err2str(msg->err));
				throw std::runtime_error("unhandled error");
			}
		}
		else {
			// Not an error.  No more messages left in this poll call
			break;
		}
	}
}


}
}
}
