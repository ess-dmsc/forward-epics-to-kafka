#include "blobs.h"
namespace BrightnESS {
namespace ForwardEpicsToKafka {
namespace blobs {

// Use raw string literal to load the config schema file
char const *schema_config_global_json{
#include "schema-config-global.json"
};
}
}
}
