#include "logger.h"
#include <cstdlib>
#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <thread>
#include <string>

int log_level = 3;

FILE * log_file = stdout;

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
Dummy dummy;


int prefix_len() {
	static int n1 = strlen(__FILE__) - 10;
	return n1;
}

void use_log_file(std::string fname) {
	FILE * f1 = fopen(fname.c_str(), "wb");
	log_file = f1;
}
