#include "blobs.h"
namespace BrightnESS {
namespace ForwardEpicsToKafka {
namespace blobs {

std::vector<char> schema_config_global_json{
#include "schema-config-global.json.cxx"
};
}
}
}
