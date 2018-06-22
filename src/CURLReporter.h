#pragma once

#if HAVE_CURL
#include <curl/curl.h>
#endif

namespace Forwarder {

/// CURLReporter is used to push metrics into InfluxDB via the HTTP endpoint.
/// It allow to easily send a message to a given URL.
/// It also provides the fact whether or not we have CURL support.

class CURLReporter {
public:
  /// Set to true if we are compiled with CURL support
  static bool const HaveCURL;
  CURLReporter();
  ~CURLReporter();
  /// Delivers a message in form of the given MemoryWriter.
  /// If CURL is not available, this is a no-op.
  void send(fmt::MemoryWriter &MemoryWriter, std::string const &URL);
};

#if HAVE_CURL
bool const CURLReporter::HaveCURL = true;

CURLReporter::CURLReporter() { curl_global_init(CURL_GLOBAL_ALL); }

CURLReporter::~CURLReporter() { curl_global_cleanup(); }

void CURLReporter::send(fmt::MemoryWriter &MemoryWriter,
                        std::string const &URL) {
  CURL *curl;
  CURLcode res;
  curl = curl_easy_init();
  if (curl) {
    curl_easy_setopt(curl, CURLOPT_URL, URL.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, MemoryWriter.c_str());
    res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
      LOG(5, "curl_easy_perform() failed: {}", curl_easy_strerror(res));
    }
  }
  curl_easy_cleanup(curl);
}

#else
bool const CURLReporter::HaveCURL = false;

CURLReporter::CURLReporter() {}

CURLReporter::~CURLReporter() {}

void CURLReporter::send(fmt::MemoryWriter &MemoryWriter,
                        std::string const &URL) {}

#endif
}
