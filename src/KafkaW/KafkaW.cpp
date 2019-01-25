#include "KafkaW.h"
#include <atomic>
#include <cerrno>

namespace KafkaW {
static_assert(RdKafka::ErrorCode::ERR_NO_ERROR == 0,
              "We rely on ERR_NO_ERROR == 0");
}
