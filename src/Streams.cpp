// SPDX-License-Identifier: BSD-2-Clause
//
// This code has been produced by the European Spallation Source
// and its partner institutes under the BSD 2 Clause License.
//
// See LICENSE.md at the top level for license information.
//
// Screaming Udder!                              https://esss.se

#include "Streams.h"
#include "Stream.h"
#include <algorithm>

namespace Forwarder {

size_t Streams::size() const {
  const std::lock_guard<std::mutex> lock(StreamsMutex);
  return StreamPointers.size();
}

void Streams::stopChannel(std::string const &channel) {
  const std::lock_guard<std::mutex> lock(StreamsMutex);
  StreamPointers.erase(
      std::remove_if(StreamPointers.begin(), StreamPointers.end(),
                     [&](std::shared_ptr<Stream> s) {
                       return (s->getChannelInfo().channel_name == channel);
                     }),
      StreamPointers.end());
}

void Streams::clearStreams() {
  Logger->trace("Main::clearStreams()  begin");
  const std::lock_guard<std::mutex> lock(StreamsMutex);
  if (!StreamPointers.empty()) {
    for (auto const &Stream : StreamPointers) {
      Stream->stop();
    }
    // Wait for Epics to cool down
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    StreamPointers.clear();
  }
  Logger->trace("Main::clearStreams()  end");
};

void Streams::checkStreamStatus() {
  const std::lock_guard<std::mutex> lock(StreamsMutex);
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

json Streams::getStreamStatuses() {
  const std::lock_guard<std::mutex> lock(StreamsMutex);

  auto StreamsArray = json::array();
  std::transform(StreamPointers.cbegin(), StreamPointers.cend(),
                 std::back_inserter(StreamsArray),
                 [](const std::shared_ptr<Stream> &CStream) {
                   return CStream->getStatusJson();
                 });

  return StreamsArray;
}

void Streams::add(std::shared_ptr<Stream> s) {
  const std::lock_guard<std::mutex> lock(StreamsMutex);
  StreamPointers.push_back(s);
}

std::shared_ptr<Stream> Streams::back() {
  return StreamPointers.empty() ? nullptr : StreamPointers.back();
}

std::shared_ptr<Stream>
Streams::getStreamByChannelName(std::string const &channel_name) {
  const std::lock_guard<std::mutex> lock(StreamsMutex);
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
