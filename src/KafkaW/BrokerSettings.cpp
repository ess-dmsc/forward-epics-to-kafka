// SPDX-License-Identifier: BSD-2-Clause
//
// This code has been produced by the European Spallation Source
// and its partner institutes under the BSD 2 Clause License.
//
// See LICENSE.md at the top level for license information.
//
// Screaming Udder!                              https://esss.se

#include "BrokerSettings.h"
#include "../logger.h"
#include "KafkaEventCb.h"
#include <librdkafka/rdkafkacpp.h>

namespace KafkaW {

void BrokerSettings::apply(RdKafka::Conf *RdKafkaConfiguration) const {
  std::string ErrorString;
  auto Logger = getLogger();
  for (const auto &ConfigurationItem : KafkaConfiguration) {
    Logger->debug("set config: {} = {}", ConfigurationItem.first,
                  ConfigurationItem.second);
    if (RdKafka::Conf::ConfResult::CONF_OK !=
        RdKafkaConfiguration->set(ConfigurationItem.first,
                                  ConfigurationItem.second, ErrorString)) {
      std::string ThrowMessage =
          fmt::format("Failure setting config: {} = {}",
                      ConfigurationItem.first, ConfigurationItem.second);
      Logger->warn(ThrowMessage.c_str());
      throw std::runtime_error(ThrowMessage);
    }
  }
}
} // namespace KafkaW
