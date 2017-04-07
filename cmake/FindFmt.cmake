# fmt not in the EPEL pinned by the dev-env crew.
# So use the source version:
find_path(path_include_fmt NAMES fmt/format.cc)
message(STATUS "path_include_fmt ${path_include_fmt}")
