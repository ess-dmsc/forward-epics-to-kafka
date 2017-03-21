#include "uri.h"
#include "logger.h"
#include <array>

namespace BrightnESS {
namespace uri {

// POD
class CG {
public:
PCRE2_SIZE a, b, n;
std::string substr(char const * p0) {
	return std::string(p0 + a, n);
}
};

class MD {
public:
MD(pcre2_match_data * md) {
	ol = pcre2_get_ovector_count(md);
	ov = pcre2_get_ovector_pointer(md);
}
CG cg(uint32_t i) {
	if (i >= ol) throw std::runtime_error("out-of-bounds capture group requested");
	return { ov[i *2+0], ov[i *2+1], ov[i *2+1] - ov[i *2+0] };
}
uint32_t ol;
PCRE2_SIZE * ov;
};


void p_regerr(int err) {
	std::array<unsigned char, 512> s1;
	auto n = pcre2_get_error_message(err, s1.data(), s1.size());
	fmt::print("err in regex: [{}, {}] {:.{}}\n", err, n, (char*)s1.data(), n);
}


static_ini::static_ini() {
	using uchar = unsigned char;
	int err = 0;
	size_t errpos = 0;
	{
		auto s1 = (uchar*) "^\\s*(([a-z]+):)?//(([-._A-Za-z0-9]+)(:([0-9]+))?)(/([-./_A-Za-z0-9]*))?\\s*$";
		auto re = pcre2_compile_8(s1, PCRE2_ZERO_TERMINATED, 0, &err, &errpos, nullptr);
		if (!re) {
			p_regerr(err);
			throw std::runtime_error("can not compile regex");
		}
		URI::re1 = re;
	}
	{
		auto s1 = (uchar*) "^\\s*(([-._A-Za-z0-9]+)(:([0-9]+))?)(/([-./_A-Za-z0-9]*))?\\s*$";
		auto re = pcre2_compile_8(s1, PCRE2_ZERO_TERMINATED, 0, &err, &errpos, nullptr);
		if (!re) {
			p_regerr(err);
			throw std::runtime_error("can not compile regex");
		}
		URI::re_host_no_slashes = re;
	}
	{
		auto s1 = (uchar*) "^/?([-./_A-Za-z0-9]*)$";
		auto re = pcre2_compile_8(s1, PCRE2_ZERO_TERMINATED, 0, &err, &errpos, nullptr);
		if (!re) {
			p_regerr(err);
			throw std::runtime_error("can not compile regex");
		}
		URI::re_no_host = re;
	}
	{
		auto s1 = (uchar*) "^/?([-._A-Za-z0-9]+)$";
		auto re = pcre2_compile_8(s1, PCRE2_ZERO_TERMINATED, 0, &err, &errpos, nullptr);
		if (!re) {
			p_regerr(err);
			throw std::runtime_error("can not compile regex");
		}
		URI::re_topic = re;
	}
}


static_ini::~static_ini() {
	if (auto & x = URI::re1) pcre2_code_free(x);
	if (auto & x = URI::re_host_no_slashes) pcre2_code_free(x);
	if (auto & x = URI::re_no_host) pcre2_code_free(x);
	if (auto & x = URI::re_topic) pcre2_code_free(x);
}


void URI::update_deps() {
	if (port != 0) {
		host_port = fmt::format("{}:{}", host, port);
	}
	else {
		host_port = host;
	}
	// check if the path could be a valid topic
	auto mdd = pcre2_match_data_create(16, nullptr);
	if (0 <= pcre2_match(re_topic, (uchar*)path.data(), path.size(), 0, 0, mdd, nullptr)) {
		topic = MD(mdd).cg(1).substr(path.data());
	}
	pcre2_match_data_free(mdd);
}


URI::~URI() {
}


URI::URI() {
}


URI::URI(std::string uri) {
	init(uri);
}


void URI::init(std::string uri) {
	using std::vector;
	using std::string;
	auto p0 = uri.data();
	auto mdd = pcre2_match_data_create(16, nullptr);
	bool match = false;
	if (!match) {
		int x;
		x = pcre2_match(re1, (uchar*)uri.data(), uri.size(), 0, 0, mdd, nullptr);
		if (x >= 0) {
			match = true;
			MD m(mdd);
			scheme = m.cg(2).substr(p0);
			host = m.cg(4).substr(p0);
			auto cg = m.cg(6);
			if (cg.n > 0) {
				port = strtoul(string(p0 + cg.a, cg.n).data(), nullptr, 10);
			}
			path = m.cg(7).substr(p0);
		}
	}
	if (!match && !require_host_slashes) {
		int x;
		x = pcre2_match(re_host_no_slashes, (uchar*)uri.data(), uri.size(), 0, 0, mdd, nullptr);
		if (x >= 0) {
			match = true;
			MD m(mdd);
			host = m.cg(2).substr(p0);
			auto cg = m.cg(4);
			if (cg.n > 0) {
				port = strtoul(string(p0 + cg.a, cg.n).data(), nullptr, 10);
			}
			path = m.cg(5).substr(p0);
		}
	}
	if (!match) {
		int x;
		x = pcre2_match(re_no_host, (uchar*)uri.data(), uri.size(), 0, 0, mdd, nullptr);
		if (x >= 0) {
			match = true;
			MD m(mdd);
			path = m.cg(0).substr(p0);
		}
	}
	pcre2_match_data_free(mdd);
	update_deps();
}


pcre2_code * URI::re1 = nullptr;
pcre2_code * URI::re_host_no_slashes = nullptr;
pcre2_code * URI::re_no_host = nullptr;
pcre2_code * URI::re_topic = nullptr;


void URI::default_port(int port_) {
	if (port == 0) {
		port = port_;
	}
	update_deps();
}


void URI::default_path(std::string path_) {
	if (path.size() == 0) {
		path = path_;
	}
	update_deps();
}


void URI::default_host(std::string host_) {
	if (host.size() == 0) {
		host = host_;
	}
	update_deps();
}


static_ini URI::compiled;


#if HAVE_GTEST
TEST(URI, host) {
	URI u1("//myhost");
	ASSERT_EQ(u1.scheme, "");
	ASSERT_EQ(u1.host, "myhost");
	ASSERT_EQ(u1.port, (uint32_t)0);
}
TEST(URI, ip) {
	URI u1("//127.0.0.1");
	ASSERT_EQ(u1.scheme, "");
	ASSERT_EQ(u1.host, "127.0.0.1");
	ASSERT_EQ(u1.port, (uint32_t)0);
}
TEST(URI, host_port) {
	URI u1("//myhost:345");
	ASSERT_EQ(u1.scheme, "");
	ASSERT_EQ(u1.host, "myhost");
	ASSERT_EQ(u1.port, (uint32_t)345);
}
TEST(URI, host_port_noslashes) {
	URI u1;
	u1.require_host_slashes = false;
	u1.init("myhost:345");
	ASSERT_EQ(u1.scheme, "");
	ASSERT_EQ(u1.host, "myhost");
	ASSERT_EQ(u1.port, (uint32_t)345);
}
TEST(URI, ip_port) {
	URI u1("//127.0.0.1:345");
	ASSERT_EQ(u1.scheme, "");
	ASSERT_EQ(u1.host, "127.0.0.1");
	ASSERT_EQ(u1.port, (uint32_t)345);
}
TEST(URI, scheme_host_port) {
	URI u1("http://my.host:345");
	u1.default_port(123);
	ASSERT_EQ(u1.scheme, "http");
	ASSERT_EQ(u1.host, "my.host");
	ASSERT_EQ(u1.host_port, "my.host:345");
	ASSERT_EQ(u1.port, (uint32_t)345);
}
TEST(URI, scheme_host_port_default) {
	URI u1("http://my.host");
	u1.default_port(123);
	ASSERT_EQ(u1.scheme, "http");
	ASSERT_EQ(u1.host, "my.host");
	ASSERT_EQ(u1.host_port, "my.host:123");
	ASSERT_EQ(u1.port, (uint32_t)123);
}
TEST(URI, scheme_host_port_pathdefault) {
	URI u1("kafka://my-host.com:8080/");
	ASSERT_EQ(u1.scheme, "kafka");
	ASSERT_EQ(u1.host, "my-host.com");
	ASSERT_EQ(u1.port, (uint32_t)8080);
	ASSERT_EQ(u1.path, "/");
}
TEST(URI, scheme_host_port_path) {
	URI u1("kafka://my-host.com:8080/som_e");
	ASSERT_EQ(u1.scheme, "kafka");
	ASSERT_EQ(u1.host, "my-host.com");
	ASSERT_EQ(u1.port, (uint32_t)8080);
	ASSERT_EQ(u1.path, "/som_e");
	ASSERT_EQ(u1.topic, "som_e");
}
TEST(URI, scheme_host_port_pathlonger) {
	URI u1("kafka://my_host.com:8080/some/longer");
	ASSERT_EQ(u1.scheme, "kafka");
	ASSERT_EQ(u1.host, "my_host.com");
	ASSERT_EQ(u1.port, (uint32_t)8080);
	ASSERT_EQ(u1.path, "/some/longer");
	ASSERT_EQ(u1.topic, "");
}
TEST(URI, host_topic) {
	URI u1("//my.host/the-topic");
	ASSERT_EQ(u1.scheme, "");
	ASSERT_EQ(u1.host, "my.host");
	ASSERT_EQ(u1.port, (uint32_t)0);
	ASSERT_EQ(u1.topic, "the-topic");
}
TEST(URI, host_port_topic) {
	URI u1("//my.host:789/the-topic");
	ASSERT_EQ(u1.scheme, "");
	ASSERT_EQ(u1.host, "my.host");
	ASSERT_EQ(u1.port, (uint32_t)789);
	ASSERT_EQ(u1.topic, "the-topic");
}
TEST(URI, abspath) {
	URI u1("/mypath/sub");
	ASSERT_EQ(u1.scheme, "");
	ASSERT_EQ(u1.host, "");
	ASSERT_EQ(u1.port, (uint32_t)0);
	ASSERT_EQ(u1.path, "/mypath/sub");
	ASSERT_EQ(u1.topic, "");
}
TEST(URI, relpath) {
	URI u1("mypath/sub");
	ASSERT_EQ(u1.scheme, "");
	ASSERT_EQ(u1.host, "");
	ASSERT_EQ(u1.port, (uint32_t)0);
	ASSERT_EQ(u1.path, "mypath/sub");
	ASSERT_EQ(u1.topic, "");
}
TEST(URI, abstopic) {
	URI u1("/topic-name.test");
	ASSERT_EQ(u1.scheme, "");
	ASSERT_EQ(u1.host, "");
	ASSERT_EQ(u1.port, (uint32_t)0);
	ASSERT_EQ(u1.path, "/topic-name.test");
	ASSERT_EQ(u1.topic, "topic-name.test");
}
TEST(URI, reltopic) {
	URI u1("topic-name.test");
	ASSERT_EQ(u1.scheme, "");
	ASSERT_EQ(u1.host, "");
	ASSERT_EQ(u1.port, (uint32_t)0);
	ASSERT_EQ(u1.path, "topic-name.test");
	ASSERT_EQ(u1.topic, "topic-name.test");
}
TEST(URI, host_default_topic) {
	URI u1("//my.host");
	u1.default_path("/some-path");
	ASSERT_EQ(u1.scheme, "");
	ASSERT_EQ(u1.host, "my.host");
	ASSERT_EQ(u1.port, (uint32_t)0);
	ASSERT_EQ(u1.path, "/some-path");
	ASSERT_EQ(u1.topic, "some-path");
}
#endif

}
}
