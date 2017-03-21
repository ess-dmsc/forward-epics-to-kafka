#pragma once

#include <string>
#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>

#if HAVE_GTEST
#include <gtest/gtest.h>
#endif

namespace BrightnESS {
namespace uri {

struct static_ini {
static_ini();
~static_ini();
};

class URI {
public:
using uchar = unsigned char;
~URI();
URI();
URI(std::string uri);
void init(std::string uri);
void default_host(std::string host);
void default_port(int port);
void default_path(std::string path);
bool is_kafka_with_topic() const;
std::string scheme;
std::string host;
std::string host_port;
uint32_t port = 0;
std::string path;
std::string topic;
bool require_host_slashes = true;
private:
static pcre2_code * re1;
static pcre2_code * re_host_no_slashes;
static pcre2_code * re_no_host;
static pcre2_code * re_topic;
static static_ini compiled;
void update_deps();
friend struct static_ini;
};

}
}
