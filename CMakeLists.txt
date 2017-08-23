cmake_minimum_required(VERSION 2.8.11)
project(forward-epics-to-kafka)

set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${PROJECT_SOURCE_DIR}/cmake")

if(EXISTS "${CMAKE_BINARY_DIR}/conanbuildinfo.cmake")
    include("${CMAKE_BINARY_DIR}/conanbuildinfo.cmake")
    conan_basic_setup()

    # Unset variables that cause files to be moved in build tree.
    unset(CMAKE_RUNTIME_OUTPUT_DIRECTORY)
    unset(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE)
    unset(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG)
    unset(CMAKE_ARCHIVE_OUTPUT_DIRECTORY)
    unset(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE)
    unset(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG)
endif()

# Yes, there exists also FindGTest.
# Yes, we do want to use this one instead:
find_package(Googletest)

add_subdirectory(src)

option(RUN_DOXYGEN "Run doxygen" TRUE)
if (RUN_DOXYGEN)
configure_file(Doxygen.conf dg.conf)
add_custom_target(docs COMMAND doxygen dg.conf DEPENDS forward-epics-to-kafka)
endif()
