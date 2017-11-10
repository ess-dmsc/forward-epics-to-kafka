#ifndef FORWARD_EPICS_TO_KAFKA_STREAMS_H
#define FORWARD_EPICS_TO_KAFKA_STREAMS_H
#include "Stream.h"
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace BrightnESS {
namespace ForwardEpicsToKafka {

class Stream;

class Streams {
private:
  std::vector<std::shared_ptr<Stream>> streams;
  std::mutex streams_mutex;

public:
  size_t size();
  void channel_stop(std::string const &channel);
  void streams_clear();
  void check_stream_status();
  void add(std::shared_ptr<Stream> s);
  std::shared_ptr<Stream> back();
  std::shared_ptr<Stream> operator[](size_t s) { return streams.at(s); };
  const std::vector<std::shared_ptr<Stream>> &get_streams();
};
}
}
#endif // FORWARD_EPICS_TO_KAFKA_STREAMS_H
