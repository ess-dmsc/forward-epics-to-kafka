# fmt not in the EPEL pinned by the dev-env crew.
# So use the source version:
find_path(FMT_INCLUDE_DIR NAMES fmt/format.cc)
set(FMT_SRC "${FMT_INCLUDE_DIR}/fmt/format.cc")

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(FMT DEFAULT_MSG
    FMT_INCLUDE_DIR
    FMT_SRC
)
