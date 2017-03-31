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

}
}
