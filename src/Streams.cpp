#include "Streams.h"
#include "Stream.h"

namespace Forwarder {

size_t Streams::size() { return streams.size(); }

void Streams::stopChannel(std::string const &channel) {
  std::unique_lock<std::mutex> lock(streams_mutex);
  streams.erase(std::remove_if(streams.begin(), streams.end(),
                               [&](std::shared_ptr<Stream> s) {
                                 return (s->channel_info().channel_name ==
                                         channel);
                               }),
                streams.end());
}

void Streams::clearStreams() {
  CLOG(7, 1, "Main::clearStreams()  begin");
  std::unique_lock<std::mutex> lock(streams_mutex);
  if (!streams.empty()) {
    for (auto x : streams) {
      x->stop();
    }
    // Wait for Epics to cool down
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    streams.clear();
  }
  CLOG(7, 1, "Main::clearStreams()  end");
};

void Streams::checkStreamStatus() {
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

void Streams::add(std::shared_ptr<Stream> s) { streams.push_back(s); }

std::shared_ptr<Stream> Streams::back() {
  return streams.empty() ? nullptr : streams.back();
}

const std::vector<std::shared_ptr<Stream>> &Streams::getStreams() {
  return streams;
}

std::shared_ptr<Stream>
Streams::getStreamByChannelName(std::string const &channel_name) {
  for (auto const &x : streams) {
    if (x->channel_info().channel_name == channel_name) {
      return x;
    }
  }
  return nullptr;
}
} // namespace Forwarder
