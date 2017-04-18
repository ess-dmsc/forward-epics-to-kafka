find_path(PCRE2_INCLUDE_DIR NAMES pcre2.h)
find_library(PCRE2_LIBRARY NAMES pcre2-8)
include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(PCRE2 DEFAULT_MSG
    PCRE2_INCLUDE_DIR
    PCRE2_LIBRARY
)
add_library(libpcre2-8 SHARED IMPORTED)
set_property(TARGET libpcre2-8 PROPERTY IMPORTED_LOCATION ${PCRE2_LIBRARY})
