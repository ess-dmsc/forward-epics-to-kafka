#pragma once

#include <fmt/format.h>
#include "logger.h"
#include <string>
#if HAVE_CURL
#include <curl/curl.h>
#endif

namespace Forwarder {
/// CURLReporter is used to push metrics into InfluxDB via the HTTP endpoint.
/// It allow to easily send a message to a given URL.
/// It also provides the fact whether or not we have CURL support.
namespace CURLReporter {
#if HAVE_CURL
struct InitCURL {
  InitCURL() { curl_global_init(CURL_GLOBAL_ALL); }
  ~InitCURL() { curl_global_cleanup(); }
};

bool const HaveCURL{true};

void send(fmt::memory_buffer &MemoryWriter, std::string const &URL) {
  static InitCURL Initializer;
  CURL *curl;
  CURLcode res;
  curl = curl_easy_init();
  if (curl) {
    curl_easy_setopt(curl, CURLOPT_URL, URL.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, MemoryWriter.data());
    res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
      getLogger()->info("curl_easy_perform() failed: {}",
                        curl_easy_strerror(res));
    }
  }
  curl_easy_cleanup(curl);
}

#else
bool const HaveCURL{false};
void send(fmt::memory_buffer &, std::string const &) {}
#endif
} // namespace CURLReporter
} // namespace Forwarder
