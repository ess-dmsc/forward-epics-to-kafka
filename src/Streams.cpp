#include "Streams.h"
#include "Stream.h"

namespace Forwarder {

/// Gets the number of streams.
///
/// \return The number of streams.
size_t Streams::size() { return streams.size(); }

/// Stop the specified channel and remove the stream.
///
/// \param channel The name of the channel to stop.
void Streams::channel_stop(std::string const &channel) {
  std::unique_lock<std::mutex> lock(streams_mutex);
  streams.erase(std::remove_if(streams.begin(), streams.end(),
                               [&](std::shared_ptr<Stream> s) {
                                 return (s->channel_info().channel_name ==
                                         channel);
                               }),
                streams.end());
}

/// Clear all the streams.
void Streams::streams_clear() {
  CLOG(7, 1, "Main::streams_clear()  begin");
  std::unique_lock<std::mutex> lock(streams_mutex);
  if (!streams.empty()) {
    for (auto x : streams) {
      x->stop();
    }
    // Wait for Epics to cool down
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    streams.clear();
  }
  CLOG(7, 1, "Main::streams_clear()  end");
};

/// Check the status of the streams and stop any that are in error.
void Streams::check_stream_status() {
  if (streams.empty()) {
    return;
  }
  streams.erase(std::remove_if(streams.begin(), streams.end(),
                               [&](std::shared_ptr<Stream> s) {
                                 if (s->status() < 0) {
                                   s->stop();
                                   return true;
                                 }
                                 return false;
                               }),
                streams.end());
}

/// Add a stream.
///
/// \param s The stream to add.
void Streams::add(std::shared_ptr<Stream> s) { streams.push_back(s); }

/// Get the last stream in the vector.
///
/// \return The last stream in the vector.
std::shared_ptr<Stream> Streams::back() {
  return streams.empty() ? nullptr : streams.back();
}

const std::vector<std::shared_ptr<Stream>> &Streams::get_streams() {
  return streams;
}

/// Return true is the given channel name is already forwarded
std::shared_ptr<Stream>
Streams::getStreamByChannelName(std::string const &channel_name) {
  for (auto const &x : streams) {
    if (x->channel_info().channel_name == channel_name) {
      return x;
    }
  }
  return {nullptr};
}
} // namespace Forwarder