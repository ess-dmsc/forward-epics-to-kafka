#include "Streams.h"

using namespace BrightnESS::ForwardEpicsToKafka::Streams;

/**
 * Gets the amount of streams in the streams vector.
 *
 * @return The size of the stream vector.
 */
int Streams::size() {
  return static_cast<int>(streams.size());
}

/**
 * Stops specified channel and erases all data in the stream.
 *
 * @param channel The channel name to stop.
 */
void Streams::channel_stop(std::string const &channel) {
  std::unique_lock<std::mutex> lock(streams_mutex);
  streams.erase(std::remove_if(streams.begin(), streams.end(), [&](std::unique_ptr<Stream>& s){
    return (s->channel_info().channel_name == channel);
  }));
}

/**
 * Clears all data from streams.
 */
void Streams::streams_clear() {
  CLOG(7, 1, "Main::streams_clear()  begin");
  std::unique_lock<std::mutex> lock(streams_mutex);
  if (!streams.empty()) {
    for (auto &x : streams) {
      x->stop();
    }
    // Wait for Epics to cool down
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    streams.clear();
  }
  CLOG(7, 1, "Main::streams_clear()  end");
};

/**
 * Stops any streams which have a status of < 0.
 */
void Streams::check_stream_status() {
  streams.erase(std::remove_if(streams.begin(), streams.end(), [&](std::unique_ptr<Stream>& s){
    if (s->status() < 0) {
      s->stop();
      return true;
    }
    return false;
  }));
}

/**
 * Add stream.
 *
 * @param s The pointer to the stream to add.
 */
void Streams::add(BrightnESS::ForwardEpicsToKafka::Stream* s){
  streams.emplace_back(s);
}

/**
 * Get last stream.
 *
 * @return A pointer to the last stream in the vector.
 */
std::unique_ptr<BrightnESS::ForwardEpicsToKafka::Stream>& Streams::back() {
  return streams.back();
}

const std::vector<std::unique_ptr<BrightnESS::ForwardEpicsToKafka::Stream>>& Streams::get_streams() {
  return streams;
}

