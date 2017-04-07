cmake_minimum_required(VERSION 2.8.11)
project(forward-epics-to-kafka)

set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${PROJECT_SOURCE_DIR}/cmake")

find_package(Googletest)

add_subdirectory(src)

option(RUN_DOXYGEN "Run doxygen" TRUE)
if (RUN_DOXYGEN)
configure_file(Doxygen.conf dg.conf)
add_custom_target(docs COMMAND doxygen dg.conf DEPENDS forward-epics-to-kafka)
endif()
