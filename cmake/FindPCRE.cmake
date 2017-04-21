find_path(path_include_pcre NAMES pcre.h)
find_library(path_library_pcre NAMES pcre)
add_library(libpcre SHARED IMPORTED)
set_property(TARGET libpcre PROPERTY IMPORTED_LOCATION ${path_library_pcre})
