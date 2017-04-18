find_path(RDKAFKA_INCLUDE_DIR NAMES librdkafka/rdkafka.h)
find_library(RDKAFKA_LIBRARIES NAMES rdkafka)
include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(RDKAFKA DEFAULT_MSG
    RDKAFKA_INCLUDE_DIR
    RDKAFKA_LIBRARIES
)
add_library(librdkafka SHARED IMPORTED)
set_property(TARGET librdkafka PROPERTY IMPORTED_LOCATION ${RDKAFKA_LIBRARIES})
