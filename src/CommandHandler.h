// SPDX-License-Identifier: BSD-2-Clause
//
// This code has been produced by the European Spallation Source
// and its partner institutes under the BSD 2 Clause License.
//
// See LICENSE.md at the top level for license information.
//
// Screaming Udder!                              https://esss.se

#pragma once
#include "logger.h"
#include "nlohmann/json.hpp"
#include <string>

namespace Forwarder {
class Forwarder;

/// Helper class to provide a callback for the Kafka command listener.
class ConfigCallback {
public:
  /// Constructor.
  ///
  /// \param main The owning class which is manipulated from the callback.
  explicit ConfigCallback(Forwarder &main);

  /// The callback entry-point.
  ///
  /// \param RawMsg The message to handle.
  void operator()(std::string const &RawMsg);

private:
  Forwarder &main;
  void handleCommand(nlohmann::json const &Msg);
  void handleCommandAdd(nlohmann::json const &Document);
  void handleCommandStopChannel(nlohmann::json const &Document);
  void handleCommandStopAll();
  void handleCommandExit();
  SharedLogger Logger = getLogger();
};

} // namespace Forwarder
