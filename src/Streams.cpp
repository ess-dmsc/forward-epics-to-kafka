#include "Streams.h"
#include "Stream.h"

namespace Forwarder {

size_t Streams::size() const { return StreamPointers.size(); }

void Streams::stopChannel(std::string const &channel) {
  std::unique_lock<std::mutex> lock(StreamsMutex);
  StreamPointers.erase(
      std::remove_if(StreamPointers.begin(), StreamPointers.end(),
                     [&](std::shared_ptr<Stream> s) {
                       return (s->getChannelInfo().channel_name == channel);
                     }),
      StreamPointers.end());
}

void Streams::clearStreams() {
  LOG(Sev::Debug, "Main::clearStreams()  begin");
  std::unique_lock<std::mutex> lock(StreamsMutex);
  if (!StreamPointers.empty()) {
    for (auto x : StreamPointers) {
      x->stop();
    }
    // Wait for Epics to cool down
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    StreamPointers.clear();
  }
  LOG(Sev::Debug, "Main::clearStreams()  end");
};

void Streams::checkStreamStatus() {
  if (StreamPointers.empty()) {
    return;
  }
  StreamPointers.erase(std::remove_if(StreamPointers.begin(),
                                      StreamPointers.end(),
                                      [&](std::shared_ptr<Stream> s) {
                                        if (s->status() < 0) {
                                          s->stop();
                                          return true;
                                        }
                                        return false;
                                      }),
                       StreamPointers.end());
}

void Streams::add(std::shared_ptr<Stream> s) { StreamPointers.push_back(s); }

std::shared_ptr<Stream> Streams::back() {
  return StreamPointers.empty() ? nullptr : StreamPointers.back();
}

const std::vector<std::shared_ptr<Stream>> &Streams::getStreams() const {
  return StreamPointers;
}

std::shared_ptr<Stream>
Streams::getStreamByChannelName(std::string const &channel_name) {
  for (auto const &x : StreamPointers) {
    if (x->getChannelInfo().channel_name == channel_name) {
      return x;
    }
  }
  return nullptr;
}
} // namespace Forwarder
