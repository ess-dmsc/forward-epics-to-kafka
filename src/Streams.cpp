#include "Streams.h"
#include "Stream.h"
#include <algorithm>

namespace Forwarder {

size_t Streams::size() const { return StreamPointers.size(); }

void Streams::stopChannel(std::string const &channel) {
  std::lock_guard<std::mutex> lock(StreamsMutex);
  StreamPointers.erase(
      std::remove_if(StreamPointers.begin(), StreamPointers.end(),
                     [&](std::shared_ptr<Stream> s) {
                       return (s->getChannelInfo().channel_name == channel);
                     }),
      StreamPointers.end());
}

void Streams::clearStreams() {
  Logger->debug("Main::clearStreams()  begin");
  std::lock_guard<std::mutex> lock(StreamsMutex);
  if (!StreamPointers.empty()) {
    for (auto const &Stream : StreamPointers) {
      Stream->stop();
    }
    // Wait for Epics to cool down
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    StreamPointers.clear();
  }
  Logger->debug("Main::clearStreams()  end");
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
  auto FoundChannel = std::find_if(
      StreamPointers.cbegin(), StreamPointers.cend(),
      [&channel_name](const std::shared_ptr<Stream> &CurrentStream) {
        return CurrentStream->getChannelInfo().channel_name == channel_name;
      });
  if (FoundChannel == StreamPointers.end()) {
    return nullptr;
  }
  return *FoundChannel;
}
} // namespace Forwarder
