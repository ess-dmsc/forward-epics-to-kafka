#include <cstdlib>
#include <cstdio>
#include <thread>
#include <vector>
#include <string>
#include <cstring>
#include <csignal>

#include <fmt/format.h>
#include "logger.h"
#include "configuration.h"
#include "MainOpt.h"
#include "Main.h"
#include "blobs.h"


namespace BrightnESS {
namespace ForwardEpicsToKafka {


}
}




void signal_handler(int signal) {
	LOG(0, "SIGNAL {}", signal);
	BrightnESS::ForwardEpicsToKafka::g__run = 0;
}



int main(int argc, char ** argv) {
	std::signal(SIGINT, signal_handler);
	std::signal(SIGTERM, signal_handler);
	auto op = BrightnESS::ForwardEpicsToKafka::parse_opt(argc, argv);
	auto & opt = *op.second;

	if (opt.log_file.size() > 0) {
		use_log_file(opt.log_file);
	}

	opt.init_logger();

	if (op.first != 0) {
		return 1;
	}

	BrightnESS::ForwardEpicsToKafka::Main main(opt);
	try {
		main.forward_epics_to_kafka();
	}
	catch (std::runtime_error & e) {
		LOG(0, "CATCH runtime error in main watchdog thread: {}", e.what());
	}
	catch (std::exception & e) {
		LOG(0, "CATCH EXCEPTION in main watchdog thread");
	}
	return 0;
}
