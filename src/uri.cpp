#include "uri.h"
#include "logger.h"
#include <iostream>

namespace Forwarder {

static std::string topic_from_path(std::string s) {
  auto p = s.find("/");
  if (p == 0) {
    s = s.substr(1);
  }
  p = s.find("/");
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

URI::URI() {}

URI::URI(std::string uri) { parse(uri); }

static bool is_alpha(std::string s) {
  for (auto c : s) {
    if (c < 'a' || c > 'z') {
      return false;
    }
  }
  return true;
}

static std::vector<std::string> protocol(std::string s) {
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

static std::vector<std::string> hostport(std::string s) {
  if (s.find("//") != 0) {
    return {std::string(), std::string(), s};
  }
  auto slash = s.find("/", 2);
  auto colon = s.find(":", 2);
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
  while (s.find(" ", a) == a) {
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

void URI::parse(std::string uri) {
  uri = trim(uri);
  auto proto = protocol(uri);
  if (!proto[0].empty()) {
    scheme = proto[0];
  }
  auto s = proto[1];
  if (!require_host_slashes) {
    if (s.find("/") != 0) {
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
}
