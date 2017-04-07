find_path(path_include_pcre2 NAMES pcre2.h)
find_library(path_library_pcre2 NAMES pcre2-8)
add_library(libpcre2-8 SHARED IMPORTED)
set_property(TARGET libpcre2-8 PROPERTY IMPORTED_LOCATION ${path_library_pcre2})
