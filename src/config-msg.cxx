#include <cstdlib>
#include <cstdio>
#include <thread>
#include <vector>
#include <algorithm>
#include <memory>
#include <string>
#include <cstring>
#include <unistd.h>
#include <getopt.h>


#include "logger.h"
#include "jansson.h"
#include <librdkafka/rdkafka.h>


class JSONMsg {
public:
json_t * json = nullptr;
char * m_msgbuf = nullptr;
std::string msgbuf() {
	if (not m_msgbuf) {
		m_msgbuf = json_dumps(json, JSON_INDENT(2) | JSON_SORT_KEYS);
	}
	return m_msgbuf;
}
~JSONMsg() {
	if (m_msgbuf) {
		free(m_msgbuf);
		m_msgbuf = nullptr;
	}
}
};




class MainOpt {
public:
std::unique_ptr<JSONMsg> msg;
int verbose = 0;
std::string channel;
std::string topic;
};



std::unique_ptr<JSONMsg> create_msg_list() {
	auto j0 = json_object();
	json_object_set(j0, "cmd", json_string("list"));
	auto ret = std::unique_ptr<JSONMsg>(new JSONMsg);
	ret->json = j0;
	return ret;
}

std::unique_ptr<JSONMsg> create_msg_add(char const * channel, char const * topic) {
	auto j0 = json_object();
	json_object_set(j0, "cmd", json_string("add"));
	json_object_set(j0, "channel", json_string(channel));
	json_object_set(j0, "topic", json_string(topic));
	auto ret = std::unique_ptr<JSONMsg>(new JSONMsg);
	ret->json = j0;
	return ret;
}

std::unique_ptr<JSONMsg> create_msg_remove(char const * channel) {
	auto j0 = json_object();
	json_object_set(j0, "cmd", json_string("remove"));
	json_object_set(j0, "channel", json_string(channel));
	auto ret = std::unique_ptr<JSONMsg>(new JSONMsg);
	ret->json = j0;
	return ret;
}



std::unique_ptr<JSONMsg> create_msg_exit() {
	auto j0 = json_object();
	json_object_set(j0, "cmd", json_string("exit"));
	auto ret = std::unique_ptr<JSONMsg>(new JSONMsg);
	ret->json = j0;
	return ret;
}






class KafkaProducer {
public:
char const * brokers = "localhost:9092";
std::string topic_name = "configuration.global";
rd_kafka_t * rk = nullptr;
rd_kafka_topic_t * rkt = nullptr;
rd_kafka_conf_t * conf = nullptr;
rd_kafka_topic_conf_t * topic_conf = nullptr;

KafkaProducer() {
	init_kafka();
}

~KafkaProducer() {
	if (rk) {
		rd_kafka_destroy(rk);
		rk = nullptr;
	}
	else if (conf) {
		// Destroy conf only if we didn't used it to create a kafka instance.
		// Even if we created the instance, can not set conf to null because we may need it.
		rd_kafka_conf_destroy(conf);
		conf = nullptr;
	}
	if (rkt) {
		rd_kafka_topic_destroy(rkt);
		rkt = nullptr;
	}
	else if (topic_conf) {
		rd_kafka_topic_conf_destroy(topic_conf);
		topic_conf = nullptr;
	}
}

static void msg_delivered_cb(
	rd_kafka_t * rk,
	const rd_kafka_message_t * rkmessage,
	void * opaque
) {
	// NOTE the opaque here is the one given during produce.

	// TODO
	// Use callback to reuse our message buffers
	LOG(1, "delivery: %s   offset %ld", rd_kafka_message_errstr(rkmessage), rkmessage->offset);
	if (rkmessage->err) {
		LOG(6, "ERROR on delivery, topic %s, %s", rd_kafka_topic_name(rkmessage->rkt), rd_kafka_err2str(rkmessage->err));
	}
	else {
		LOG(0, "OK delivered (%zd bytes, offset %ld, partition %d): %.*s\n",
			rkmessage->len, rkmessage->offset, rkmessage->partition, (int)rkmessage->len, (const char *)rkmessage->payload);
	}
}


static void kafka_error_cb(rd_kafka_t * rk, int err_i, const char * reason, void * opaque) {
	// cast necessary because of Kafka API design
	rd_kafka_resp_err_t err = (rd_kafka_resp_err_t) err_i;
	LOG(7, "ERROR Kafka: %d, %s, %s, %s", err_i, rd_kafka_err2name(err), rd_kafka_err2str(err), reason);

	// Can not throw, as it's Kafka's thread.
	// Must notify my watchdog though.
	//auto ins = reinterpret_cast<KafkaOpaqueType*>(opaque);
	//ins->error_from_kafka_callback();
}



static int stats_cb(rd_kafka_t * rk, char * json, size_t json_len, void * opaque) {
	LOG(3, "INFO stats_cb length %d", json_len);
	// TODO
	// What does Kafka want us to return from this callback?
	return 0;
}



void init_kafka() {
	int const errstr_N = 512;
	char errstr[errstr_N];

	conf = rd_kafka_conf_new();
	rd_kafka_conf_set_dr_msg_cb(conf, &KafkaProducer::msg_delivered_cb);
	rd_kafka_conf_set_error_cb(conf, &KafkaProducer::kafka_error_cb);
	rd_kafka_conf_set_stats_cb(conf, &KafkaProducer::stats_cb);

	rd_kafka_conf_set(conf, "message.max.bytes", "100000", errstr, errstr_N);
	rd_kafka_conf_set(conf, "fetch.message.max.bytes", "100000", errstr, errstr_N);
	rd_kafka_conf_set(conf, "statistics.interval.ms", "10000", errstr, errstr_N);
	rd_kafka_conf_set(conf, "metadata.request.timeout.ms", "2000", errstr, errstr_N);
	rd_kafka_conf_set(conf, "socket.timeout.ms", "2000", errstr, errstr_N);
	rd_kafka_conf_set(conf, "session.timeout.ms", "2000", errstr, errstr_N);

	rk = rd_kafka_new(RD_KAFKA_PRODUCER, conf, errstr, errstr_N);
	if (!rk) {
		LOG(7, "ERROR can not create kafka handle: %s", errstr);
		throw std::runtime_error("can not create Kafka handle");
	}
	LOG(3, "Name of the new Kafka handle: %s", rd_kafka_name(rk));
	rd_kafka_set_log_level(rk, 10);
	if (rd_kafka_brokers_add(rk, brokers) == 0) {
		LOG(7, "ERROR could not add brokers");
		throw std::runtime_error("can not add brokers");
	}

	topic_conf = rd_kafka_topic_conf_new();
	rd_kafka_topic_conf_set(topic_conf, "produce.offset.report", "true", errstr, errstr_N);
	rd_kafka_topic_conf_set(topic_conf, "message.timeout.ms", "2000", errstr, errstr_N);

	rkt = rd_kafka_topic_new(rk, topic_name.c_str(), topic_conf);
	if (rkt == nullptr) {
		// Seems like Kafka uses the system error code?
		auto errstr = rd_kafka_err2str(rd_kafka_errno2err(errno));
		LOG(7, "ERROR could not create Kafka topic: %s", errstr);
		throw std::runtime_error("can not create kafka topic");
	}
	LOG(1, "OK, seems like we've created topic %s", rd_kafka_topic_name(rkt));
	//rd_kafka_poll(rk, 10);
}


void msg(std::string msg) {
	if (not rk or not rkt) {
		throw std::runtime_error("kafka is not initialized");
	}
	int x;
	auto partition = RD_KAFKA_PARTITION_UA;
	partition = 0;

	void const * key = NULL;
	size_t key_len = 0;

	void * callback_data = NULL;
	int msgflags = RD_KAFKA_MSG_F_COPY; // 0, RD_KAFKA_MSG_F_COPY, RD_KAFKA_MSG_F_FREE

	x = rd_kafka_produce(rkt, partition, msgflags, (void*)msg.c_str(), msg.size(), key, key_len, callback_data);
	if (x != 0) {
		LOG(7, "ERROR on produce topic %s  partition %i: %s", rd_kafka_topic_name(rkt), partition, rd_kafka_err2str(rd_kafka_last_error()));
		throw std::runtime_error("ERROR on message send");
	}

	LOG(1, "produced for topic %s partition %i", rd_kafka_topic_name(rkt), partition);
}

void wait_queue_out() {
	while (rd_kafka_outq_len(rk)) {
		rd_kafka_poll(rk, 50);
	}
}

};





int main(int argc, char ** argv) {
	MainOpt opt;
	static struct option long_options[] = {
		{"verbose", no_argument,        &opt.verbose, 1},
		{"list",    no_argument,        0,  0 },
		{"add",     no_argument,        0,  0 },
		{"remove",  no_argument,        0,  0 },
		{"exit",    no_argument,        0,  0 },
		{"channel", required_argument,  0,  0 },
		{"topic",   required_argument,  0,  0 },
		{0, 0, 0, 0},
	};
	std::string cmd;
	int option_index = 0;
	while (true) {
		int c = getopt_long(argc, argv, "", long_options, &option_index);
		if (c == -1) break;
		//printf("at option %s\n", long_options[option_index].name);
		auto lname = long_options[option_index].name;
		switch (c) {
		case 0:
			// long option without short equivalent:
			if (std::string("topic") == lname) {
				opt.topic = optarg;
			}
			if (std::string("channel") == lname) {
				opt.channel = optarg;
			}
			if (std::string("list") == lname) {
				cmd = lname;
			}
			if (std::string("add") == lname) {
				cmd = lname;
			}
			if (std::string("remove") == lname) {
				cmd = lname;
			}
			if (std::string("exit") == lname) {
				cmd = lname;
			}
		}
	}
	if (true && optind < argc) {
		printf("Left-over options?\n");
		for (int i1 = optind; i1 < argc; ++i1) {
			printf("%2d %s\n", i1, argv[i1]);
		}
	}

	if (cmd == "list") {
		opt.msg = create_msg_list();
	}
	else if (cmd == "add") {
		opt.msg = create_msg_add(opt.channel.c_str(), opt.topic.c_str());
	}
	else if (cmd == "remove") {
		opt.msg = create_msg_remove(opt.channel.c_str());
	}
	else if (cmd == "exit") {
		opt.msg = create_msg_exit();
	}

	KafkaProducer p;
	if (opt.msg) {
		p.msg(opt.msg->msgbuf());
	}
	p.wait_queue_out();
	return 0;
}
