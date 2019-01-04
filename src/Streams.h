#pragma once

#include "Stream.h"
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace Forwarder {

class Stream;

class Streams {
private:
  std::vector<std::shared_ptr<Stream>> StreamPointers;
  std::mutex StreamsMutex;

public:
  /// Gets the number of streams.
  ///
  /// \return The number of streams.
  size_t size() const;

  /// Stop the specified channel and remove the stream.
  ///
  /// \param channel The name of the channel to stop.
  void stopChannel(std::string const &channel);

  /// Clear all the streams.
  void clearStreams();

  /// Check the status of the streams and stop any that are in error.
  void checkStreamStatus();

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
  ///
  /// \return The last stream in the vector.
  std::shared_ptr<Stream> back();
  std::shared_ptr<Stream> operator[](size_t s) { return StreamPointers.at(s); };
  const std::vector<std::shared_ptr<Stream>> &getStreams() const;
};
} // namespace Forwarder
