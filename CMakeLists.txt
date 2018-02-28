cmake_minimum_required(VERSION 2.8.11)
project(forward-epics-to-kafka)

set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${PROJECT_SOURCE_DIR}/cmake")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -fPIC -g")

if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS "4.9.3")
else ()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fdiagnostics-color=auto")
endif()

if (WIN32)
    add_definitions(-DNOGDI -DNOMINMAX)
endif(WIN32)

if(CMAKE_COMPILER_IS_GNUCXX AND COV)
    include(CodeCoverage)
    set(exclude_from_coverage "*_generated.h" "src/date/*" "*/.conan/data/*")
    setup_target_for_coverage(coverage tests coverage ${exclude_from_coverage})
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_COVERAGE}")
endif()

# Simplify compiler version to, at most, major and minor version
string(REPLACE "." ";" COMPILER_VERSION_LIST ${CMAKE_CXX_COMPILER_VERSION})
list(LENGTH COMPILER_VERSION_LIST COMPILER_VERSION_LENGTH)
if (COMPILER_VERSION_LENGTH GREATER 2)
    list(GET COMPILER_VERSION_LIST 0 COMPILER_MAJOR_VERSION)
    list(GET COMPILER_VERSION_LIST 1 COMPILER_MINOR_VERSION)
    set(SIMPLE_COMPILER_VERSION ${COMPILER_MAJOR_VERSION}.${COMPILER_MINOR_VERSION})
else()
    set(SIMPLE_COMPILER_VERSION ${CMAKE_CXX_COMPILER_VERSION})
endif()

if(NOT CONAN_DISABLE)
    include(${CMAKE_MODULE_PATH}/conan.cmake)
    conan_cmake_run(CONANFILE conan/conanfile.txt
            BASIC_SETUP NO_OUTPUT_DIRS
            BUILD_TYPE "None"
            BUILD outdated
            SETTINGS "compiler.version=${SIMPLE_COMPILER_VERSION}")
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
