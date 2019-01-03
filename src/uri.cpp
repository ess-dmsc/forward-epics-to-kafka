#include "uri.h"
#include "logger.h"
#include <algorithm>
#include <ciso646>
#include <iostream>

namespace Forwarder {

static std::string topic_from_path(std::string s) {
  if (s.compare(0, 1, "/") == 0) {
    s = s.substr(1);
  }
  auto p = s.find('/');
  if (p == std::string::npos) {
    return s;
  } else {
    if (p == 0) {
      return s.substr(1);
    } else {
      return std::string();
    }
  }
}

void URI::update_deps() {
  if (port != 0) {
    host_port = fmt::format("{}:{}", host, port);
  } else {
    host_port = host;
  }
  auto t = topic_from_path(path);
  if (!t.empty()) {
    topic = t;
  }
}

URI::URI(std::string const &Uri) { parse(Uri); }

static bool is_alpha(std::string const &s) {
  return not std::any_of(s.cbegin(), s.cend(), [](const char &Character) {
    return Character < 'a' or Character > 'z';
  });
}

static std::vector<std::string> protocol(std::string const &s) {
  auto slashes = s.find("://");
  if (slashes == std::string::npos || slashes == 0) {
    return {std::string(), s};
  }
  auto proto = s.substr(0, slashes);
  if (!is_alpha(proto)) {
    return {std::string(), s};
  }
  return {proto, s.substr(slashes + 1, std::string::npos)};
}

static std::vector<std::string> hostport(std::string const &s) {
  /// \note This code REALLY needs error handling.
  if (s.compare(0, 2, "//") != 0) {
    return {std::string(), std::string(), s};
  }
  auto slash = s.find('/', 2);
  auto colon = s.find(':', 2);
  if (colon == std::string::npos) {
    if (slash == std::string::npos) {
      return {s.substr(2), std::string(), std::string()};
    } else {
      return {s.substr(2, slash - 2), std::string(), s.substr(slash)};
    }
  } else {
    if (slash == std::string::npos) {
      return {s.substr(2, colon - 2), s.substr(colon + 1), std::string()};
    } else {
      if (colon < slash) {
        return {s.substr(2, colon - 2), s.substr(colon + 1, slash - colon - 1),
                s.substr(slash)};
      } else {
        return {s.substr(2, slash - 2), std::string(), s.substr(slash)};
      }
    }
  }
  return {std::string(), std::string(), s};
}

static std::string trim(std::string s) {
  std::string::size_type a = 0;
  while (s.find(' ', a) == a) {
    ++a;
  }
  s = s.substr(a);
  if (s.empty()) {
    return s;
  }
  a = s.size() - 1;
  while (s[a] == ' ') {
    --a;
  }
  s = s.substr(0, a + 1);
  return s;
}

void URI::parse(std::string Uri) {
  Uri = trim(Uri);
  auto proto = protocol(Uri);
  if (!proto[0].empty()) {
    scheme = proto[0];
  }
  auto s = proto[1];
  if (!require_host_slashes) {
    if (s.find('/') != 0) {
      s = "//" + s;
    }
  }
  auto hp = hostport(s);
  if (!hp[0].empty()) {
    host = hp[0];
  }
  if (!hp[1].empty()) {
    port = strtoul(hp[1].data(), nullptr, 10);
  }
  if (!hp[2].empty()) {
    path = hp[2];
  }
  update_deps();
}
} // namespace Forwarder
