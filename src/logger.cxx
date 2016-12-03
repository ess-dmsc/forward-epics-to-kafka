#include "logger.h"
#include <cstdlib>
#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <thread>
#include <string>

int log_level = 3;

static FILE * log_file = stdout;

class Dummy {
public:
Dummy() {
	log_file = stdout;
}
~Dummy() {
	if (log_file != nullptr and log_file != stdout) {
		LOG(9, "Closing log");
		fclose(log_file);
	}
}
};
static Dummy g__logger__dummy;


int prefix_len() {
	static int n1 = strlen(__FILE__) - 10;
	return n1;
}

void use_log_file(std::string fname) {
	FILE * f1 = fopen(fname.c_str(), "wb");
	log_file = f1;
}

void dwlog_inner(int level, char const * file, int line, char const * func, std::string const & s1) {
	int npre = prefix_len();
	int const n2 = strlen(file);
	if (npre > n2) {
		fmt::print(log_file, "ERROR in logging API: npre > n2\n");
		npre = 0;
	}
	auto f1 = file + npre;
	try {
		fmt::print(log_file, "{}:{} [{}]:  {}\n", f1, line, level, s1);
	}
	catch (fmt::FormatError & e) {
		fmt::print(log_file, "ERROR  fmt::FormatError {}:{}\n", f1, line);
	}
	//fflush(log_file);
}
