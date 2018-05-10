#include "logger.h"
#include "KafkaW/KafkaW.h"
#include <atomic>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#ifdef _MSC_VER
#include <io.h>
#include <iso646.h>
#define isatty _isatty
#else
#include <unistd.h>
#endif
#include <atomic>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <thread>
#ifdef HAVE_GRAYLOG_LOGGER
#include <graylog_logger/GraylogInterface.hpp>
#include <graylog_logger/Log.hpp>
#endif

int log_level = 3;

// adhoc namespace because it would now collide with ::Logger defined
// in gralog_logger

namespace DW {

class Logger {
public:
  Logger();
  ~Logger();
  void use_log_file(std::string fname);
  void log_kafka_gelf_start(std::string broker, std::string topic);
  void log_kafka_gelf_stop();
  FILE *log_file = stdout;
  int is_tty = 1;
  void dwlog_inner(int level, int color, char const *file, int line,
                   char const *func, std::string const &s1);
  int prefix_len();
  void fwd_graylog_logger_enable(std::string address);

private:
  std::atomic<bool> do_run_kafka{false};
  std::atomic<bool> do_use_graylog_logger{false};
  std::shared_ptr<KafkaW::Producer> producer;
  std::unique_ptr<KafkaW::Producer::Topic> topic;
  std::thread thread_poll;
};

Logger::Logger() { is_tty = isatty(fileno(log_file)); }

Logger::~Logger() {
  do_run_kafka = false;
  if (log_file != nullptr and log_file != stdout) {
    LOG(0, "Closing log");
    fclose(log_file);
  }
  if (thread_poll.joinable()) {
    thread_poll.join();
  }
}

void Logger::use_log_file(std::string fname) {
  FILE *f1 = fopen(fname.c_str(), "wb");
  log_file = f1;
  is_tty = isatty(fileno(log_file));
}

void Logger::log_kafka_gelf_start(std::string address, std::string topicname) {
  KafkaW::BrokerSettings BrokerSettings;
  BrokerSettings.Address = address;
  producer.reset(new KafkaW::Producer(BrokerSettings));
  topic.reset(new KafkaW::Producer::Topic(producer, topicname));
  topic->enableCopy();
  thread_poll = std::thread([this] {
    while (do_run_kafka.load()) {
      producer->poll();
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
  });
  do_run_kafka = true;
}

void Logger::log_kafka_gelf_stop() { do_run_kafka = false; }

void Logger::fwd_graylog_logger_enable(std::string address) {
#ifdef HAVE_GRAYLOG_LOGGER
  auto addr = address;
  int port = 12201;
  auto col = address.find(":");
  if (col != std::string::npos) {
    addr = address.substr(0, col);
    port = strtol(address.c_str() + col + 1, nullptr, 10);
  }
  Log::RemoveAllHandlers();
  LOG(4, "Enable graylog_logger on {}:{}", addr, port);
  Log::AddLogHandler(new GraylogInterface(addr, port));
  do_use_graylog_logger = true;
#else
  LOG(0, "ERROR not compiled with support for graylog_logger");
#endif
}

void Logger::dwlog_inner(int level, int color, char const *file, int line,
                         char const *func, std::string const &s1) {
  int npre = prefix_len();
  int const n2 = strlen(file);
  if (npre > n2) {
    npre = 0;
  }
  auto f1 = file + npre;
  {
    // only use color for stdout
    std::string lmsg;
    if (is_tty && color > 0 && color < 8) {
      static char const *cols[]{
          "",
          "\x1b[107;1;31m",
          "\x1b[100;1;33m",
          "\x1b[107;1;35m",
          "\x1b[107;1;36m",
          "\x1b[107;1;34m",
          "\x1b[107;1;32m",
          "\x1b[107;1;30m",
      };
      lmsg = fmt::format("{}:{} [{}]:  {}{}\x1b[0m\n", f1, line, level,
                         cols[color], s1);
    } else {
      lmsg = fmt::format("{}:{} [{}]:  {}\n", f1, line, level, s1);
    }
    fwrite(lmsg.c_str(), 1, lmsg.size(), log_file);
  }
  if (level < 7 && do_run_kafka.load()) {
    // Format again without color
    auto lmsg = fmt::format("{}:{} [{}]:  {}\n", f1, line, level, s1);
    // If we will use logging to Kafka in the future, refactor a bit to reduce
    // duplicate work..
    using nlohmann::json;
    auto Document = json::object();
    Document["version"] = "1.1";
    Document["short_message"] = lmsg;
    Document["level"] = level;
    Document["_FILE"] = file;
    Document["_LINE"] = line;
    auto DocumentString = Document.dump();
    topic->produce((KafkaW::uchar *)DocumentString.c_str(),
                   DocumentString.size(), true);
  }
#ifdef HAVE_GRAYLOG_LOGGER
  if (do_use_graylog_logger.load() and level < 7) {
    // Format again without color
    auto lmsg = fmt::format("{}:{} [{}]:  {}\n", f1, line, level, s1);
    Log::Msg(level, lmsg);
  }
#endif
}

int Logger::prefix_len() {
  static int n1 = strlen(__FILE__) - 10;
  return n1;
}

static Logger g__logger;
}

void use_log_file(std::string fname) { DW::g__logger.use_log_file(fname); }

void dwlog_inner(int level, int c, char const *file, int line, char const *func,
                 std::string const &s1) {
  DW::g__logger.dwlog_inner(level, c, file, line, func, s1);
}

void log_kafka_gelf_start(std::string broker, std::string topic) {
  DW::g__logger.log_kafka_gelf_start(broker, topic);
}

void fwd_graylog_logger_enable(std::string address) {
  DW::g__logger.fwd_graylog_logger_enable(address);
}
