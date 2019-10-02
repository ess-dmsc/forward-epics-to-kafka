// SPDX-License-Identifier: BSD-2-Clause
//
// This code has been produced by the European Spallation Source
// and its partner institutes under the BSD 2 Clause License.
//
// See LICENSE.md at the top level for license information.
//
// Screaming Udder!                              https://esss.se

#include "json.h"

JsonMaybe<nlohmann::json> find_array(std::string Key,
                                     nlohmann::json const &Json) {
  using T = nlohmann::json;
  if (!Json.is_object()) {
    return JsonMaybe<T>();
  }
  auto It = Json.find(Key);
  if (It == Json.end()) {
    return JsonMaybe<T>();
  }
  if (!It.value().is_array()) {
    return JsonMaybe<T>();
  }
  return JsonMaybe<T>(It.value());
}
