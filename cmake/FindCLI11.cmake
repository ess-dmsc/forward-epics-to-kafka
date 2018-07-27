find_path(CLI11_INCLUDE_DIR NAMES CLI/CLI.hpp)
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(CLI11 DEFAULT_MSG
    CLI11_INCLUDE_DIR
)
