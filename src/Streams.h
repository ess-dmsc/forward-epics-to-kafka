#ifndef FORWARD_EPICS_TO_KAFKA_STREAMS_H
#define FORWARD_EPICS_TO_KAFKA_STREAMS_H
#include "Stream.h"

namespace BrightnESS {
namespace ForwardEpicsToKafka {
namespace Streams{

class Streams {
private:
  std::vector<std::unique_ptr<Stream>> streams;
  std::mutex streams_mutex;
public:
  int size();
  void channel_stop(std::string const &channel);
  void streams_clear();
  void check_stream_status();
  void add(Stream *s);
  std::unique_ptr<Stream> & back();
  std::unique_ptr<Stream>& operator[](size_t s){return streams.at(s);};
  const std::vector<std::unique_ptr<Stream>>& get_streams();
};
}
}
}
#endif //FORWARD_EPICS_TO_KAFKA_STREAMS_H
