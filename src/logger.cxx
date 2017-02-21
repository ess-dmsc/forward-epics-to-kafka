#include "logger.h"
#include <cstdlib>
#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <atomic>
#include <memory>
#include <thread>
#include <string>
#include "KafkaW.h"
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

int log_level = 3;

class Logger {
public:
~Logger();
void use_log_file(std::string fname);
void log_kafka_gelf_start(std::string broker, std::string topic);
void log_kafka_gelf_stop();
FILE * log_file = stdout;
void dwlog_inner(int level, char const * file, int line, char const * func, std::string const & s1);
int prefix_len();
private:
std::atomic<bool> do_run_kafka { false };
std::unique_ptr<KafkaW::Producer> producer;
std::unique_ptr<KafkaW::Producer::Topic> topic;
std::thread thread_poll;
};

Logger::~Logger() {
	if (log_file != nullptr and log_file != stdout) {
		LOG(9, "Closing log");
		fclose(log_file);
	}
}

void Logger::use_log_file(std::string fname) {
	FILE * f1 = fopen(fname.c_str(), "wb");
	log_file = f1;
}

void Logger::log_kafka_gelf_start(std::string address, std::string topicname) {
	KafkaW::BrokerOpt opt;
	opt.address = address;
	producer.reset(new KafkaW::Producer(opt));
	topic.reset(new KafkaW::Producer::Topic(*producer, topicname));
	thread_poll = std::thread([this]{
		while (do_run_kafka.load()) {
			producer->poll();
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
	});
	do_run_kafka = true;
}

void Logger::log_kafka_gelf_stop() {
	do_run_kafka = false;
	// Wait a bit with the cleanup...
	//auto t = topic.exchange(nullptr);
	//auto p = producer.exchange(nullptr);
}

void Logger::dwlog_inner(int level, char const * file, int line, char const * func, std::string const & s1) {
	int npre = prefix_len();
	int const n2 = strlen(file);
	if (npre > n2) {
		//fmt::print(log_file, "ERROR in logging API: npre > n2\n");
		npre = 0;
	}
	auto f1 = file + npre;
	fmt::print(log_file, "{}:{} [{}]:  {}\n", f1, line, level, s1);
	if (level > 1 && do_run_kafka.load()) {
		// If we will use logging to Kafka in the future, refactor a bit to reduce duplicate work..
		auto lmsg = fmt::format("{}:{} [{}]:  {}\n", f1, line, level, s1);
		using namespace rapidjson;
		Document d;
		auto & a = d.GetAllocator();
		d.SetObject();
		d.AddMember("version", "1.1", a);
		d.AddMember("short_message", Value(lmsg.c_str(), a), a);
		d.AddMember("level", Value(level), a);
		d.AddMember("_FILE", Value(file, a), a);
		d.AddMember("_LINE", Value(line), a);
		StringBuffer buf1;
		Writer<StringBuffer> wr(buf1);
		d.Accept(wr);
		auto s1 = buf1.GetString();
		topic->produce((void*)s1, strlen(s1), nullptr, true);
	}
	//fflush(log_file);
}

int Logger::prefix_len() {
	static int n1 = strlen(__FILE__) - 10;
	return n1;
}

static Logger g__logger;


void use_log_file(std::string fname) {
	g__logger.use_log_file(fname);
}

void dwlog_inner(int level, char const * file, int line, char const * func, std::string const & s1) {
	g__logger.dwlog_inner(level, file, line, func, s1);
}

void log_kafka_gelf_start(std::string broker, std::string topic) {
	g__logger.log_kafka_gelf_start(broker, topic);
}

void log_kafka_gelf_stop() {
}
