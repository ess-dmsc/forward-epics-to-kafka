cmake_minimum_required(VERSION 3.1.0)
project(forward-epics-to-kafka)

set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${PROJECT_SOURCE_DIR}/cmake")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS "4.9.3")
else ()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fdiagnostics-color=auto")
endif()

if (WIN32)
    add_definitions(-DNOGDI -DNOMINMAX)
endif(WIN32)

if(CMAKE_COMPILER_IS_GNUCXX AND COV)
    include(CodeCoverage)
    setup_target_for_coverage(coverage tests coverage})
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_COVERAGE}")
endif()

SET(CONAN_PROFILE "default" CACHE STRING "Name of conan profile to use, uses default by default")
SET(CONAN "AUTO" CACHE STRING "conan options AUTO (conan must be in path), MANUAL (expects conanbuildinfo.cmake in build directory) or DISABLE")

if(CMAKE_BUILD_TYPE MATCHES "Release")
  set(OUTPUT_DIRS "")
else()
  set(OUTPUT_DIRS "NO_OUTPUT_DIRS")
endif()

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(${OUTPUT_DIRS} SKIP_RPATH)

# for static build etc.
set(FETK_EXTRA_LIBRARIES "" CACHE STRING "Extra Libraries")
set(FETK_BUILD_TYPE "SHARED" CACHE STRING "Build Type")

if (${FETK_BUILD_TYPE} STREQUAL "STATIC")
    if(CONAN_DISABLE)
          foreach(flag_var
                CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE
                CMAKE_C_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELWITHDEBINFO
                CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
                CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
            if(${flag_var} MATCHES "/MD")
              string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
            endif()
          endforeach()
          add_definitions(-DLIBRDKAFKA_STATICLIB -DEPICS_DLL_NO)
    else()
        MESSAGE(FATAL_ERROR "Static builds are not yet supported by our conan packages")
    endif(CONAN_DISABLE)
endif()

# Yes, there exists also FindGTest.
# Yes, we do want to use this one instead:
if(NOT GOOGLETEST_DISABLE)
find_package(Googletest)
endif()

add_subdirectory(src)

option(RUN_DOXYGEN "Run doxygen" FALSE)
if (RUN_DOXYGEN)
configure_file(Doxygen.conf dg.conf)
add_custom_target(docs COMMAND doxygen dg.conf DEPENDS forward-epics-to-kafka)
endif()

