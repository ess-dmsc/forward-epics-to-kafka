// SPDX-License-Identifier: BSD-2-Clause
//
// This code has been produced by the European Spallation Source
// and its partner institutes under the BSD 2 Clause License.
//
// See LICENSE.md at the top level for license information.
//
// Screaming Udder!                              https://esss.se

#pragma once

#include "Stream.h"
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace Forwarder {

using nlohmann::json;

class Stream;

class Streams {

public:
  /// Gets the number of streams.
  ///
  /// \return The number of streams.
  size_t size();

  /// Stop the specified channel and remove the stream.
  ///
  /// \param channel The name of the channel to stop.
  void stopChannel(std::string const &channel);

  /// Clear all the streams.
  void clearStreams();

  /// Check the status of the streams and stop any that are in error.
  void checkStreamStatus();

  json getStreamStatuses();

  /// Add a stream.
  ///
  /// \param s The stream to add.
  void add(std::shared_ptr<Stream> s);

  /// Get a stream by name.
  ///
  /// \param channel_name The channel name.
  /// \return The stream if found otherwise nullptr.
  std::shared_ptr<Stream>
  getStreamByChannelName(std::string const &channel_name);

  /// Get the last stream in the vector.
  /// NOT THREAD SAFE
  /// \return The last stream in the vector.
  std::shared_ptr<Stream> back();
  std::shared_ptr<Stream> operator[](size_t s) { return StreamPointers.at(s); };

private:
  std::vector<std::shared_ptr<Stream>> StreamPointers;
  std::mutex StreamsMutex;
  SharedLogger Logger = getLogger();
};
} // namespace Forwarder
