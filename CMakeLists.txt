cmake_minimum_required(VERSION 3.1.0)
project(forward-epics-to-kafka)

set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${PROJECT_SOURCE_DIR}/cmake")

include(${CMAKE_MODULE_PATH}/Versioning.cmake)
set_version()
set(VERSION_INCLUDE_DIR ${CMAKE_BINARY_DIR}/version)
create_version_header(
  ${CMAKE_MODULE_PATH}/templates/Version.h.in
  ${VERSION_INCLUDE_DIR}/Version.h
)

set (CMAKE_CXX_STANDARD 14)

if (WIN32)
    add_definitions(-DNOGDI -DNOMINMAX)
else()
    if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS "4.9.3")
    else ()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fdiagnostics-color=auto")
    endif()
    # Set a relative RPATH on Mac and Linux to use libraries in the <BUILD_DIR>/lib directory
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-rpath,'\\\$ORIGIN/../lib'")
    if (CMAKE_COMPILER_IS_GNUCXX)
        # Stop gcc linker (version 7 onwards) from overriding rpath with runpath in the dynamic table
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--disable-new-dtags")
    endif()
endif(WIN32)

# Test Coverage - with gcc only and enable with -DCOV
if(CMAKE_COMPILER_IS_GNUCXX AND COV)
    include(CodeCoverage)
    setup_target_for_coverage(coverage tests coverage})
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_COVERAGE}")
endif()


# Conan - bring in dependencies with conan
set(CONAN_PROFILE "default" CACHE STRING "Name of conan profile to use, uses default by default")
set(CONAN "AUTO" CACHE STRING "conan options AUTO (conan must be in path), MANUAL (expects conanbuildinfo.cmake in build directory) or DISABLE")

if(WIN32)
    set(CONAN_FILE conan/conanfile_win32.txt)
else()
    set(CONAN_FILE conan/conanfile.txt)
endif(WIN32)

if(${CONAN} MATCHES "AUTO")
    include(${CMAKE_MODULE_PATH}/conan.cmake)
    conan_cmake_run(CONANFILE ${CONAN_FILE}
            PROFILE ${CONAN_PROFILE}
            BASIC_SETUP KEEP_RPATHS CMAKE_TARGETS
            BUILD_TYPE "None"
            BUILD outdated)
elseif(${CONAN} MATCHES "MANUAL")
    if(EXISTS "${CMAKE_BINARY_DIR}/conanbuildinfo.cmake")
        include("${CMAKE_BINARY_DIR}/conanbuildinfo.cmake")
        conan_basic_setup(${OUTPUT_DIRS} KEEP_RPATHS TARGETS)
    else()
        message(FATAL_ERROR "CONAN set to MANUAL but no file named conanbuildinfo.cmake found in build directory")
    endif()
elseif(NOT ${CONAN} MATCHES "DISABLE")
    message(FATAL_ERROR "Unrecognised option for CONAN, use AUTO, MANUAL or DISABLE")
endif()


# Add our src directory
add_subdirectory(src)


# Doxygen documentation
option(RUN_DOXYGEN "Run doxygen" FALSE)
if (RUN_DOXYGEN)
    configure_file(Doxygen.conf dg.conf)
    add_custom_target(docs COMMAND doxygen dg.conf DEPENDS forward-epics-to-kafka)
endif()
