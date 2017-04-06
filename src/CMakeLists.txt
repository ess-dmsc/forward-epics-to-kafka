set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ..)

message(STATUS "CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")

file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/git_commit_current.cxx "extern \"C\" char const GIT_COMMIT[] = \"NOTSET\";\n")

add_custom_target(git_commit_now ALL
COMMAND echo 'extern \"C\" char const GIT_COMMIT[] = \"'`git rev-parse HEAD`'\"\;' > ${CMAKE_CURRENT_BINARY_DIR}/git_commit_now
WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_custom_target(git_commit_current ALL
COMMAND git diff --name-only --no-index ${CMAKE_CURRENT_BINARY_DIR}/git_commit_now ${CMAKE_CURRENT_BINARY_DIR}/git_commit_current.cxx || cp ${CMAKE_CURRENT_BINARY_DIR}/git_commit_now ${CMAKE_CURRENT_BINARY_DIR}/git_commit_current.cxx
DEPENDS git_commit_now
)

# According to the official ESS wiki documentation, the base path to the
# EPICS v4 modules is given by EPICS_MODULES_PATH
# Also, all EPICS bases can be found in EPICS_BASES_PATH

# Even though not mentioned on the wiki, the official ESS EPICS installation
# provides the version of the EPICS base as used by EPICS v4
# in EPICS_V4_BASE_VERSION

set(epics_base_version "$ENV{EPICS_V4_BASE_VERSION}")
set(epicsbase_dir      ".")
if (DEFINED ENV{EPICS_BASES_PATH})
set(epicsbase_dir      "$ENV{EPICS_BASES_PATH}/base-${epics_base_version}")
endif()
set(epics_arch         "$ENV{EPICS_HOST_ARCH}")
set(epicsv4_dir        ".")
if (DEFINED ENV{EPICS_MODULES_PATH})
set(epicsv4_dir        "$ENV{EPICS_MODULES_PATH}")
endif()

# Currently, the official environment gives no hint about which specific
# version of the modules we should use.  We therefore pin it to the ones
# which are currently considered as 'production' for our purposes.

set(epics_pvData_version 5.0.2 CACHE STRING "pvData version")
set(epics_pvAccess_version 4.1.2 CACHE STRING "pvAccess version")
set(epics_pvDatabase_version 4.1.1 CACHE STRING "pvDatabase version")
set(epics_normativeTypes_version 5.0.2 CACHE STRING "normativeTypes version")
set(USE_GRAYLOG_LOGGER TRUE CACHE BOOL "Try to use graylog_logger")

# fmt not in the EPEL pinned by the dev-env crew.
# So use the source version:
find_path(path_include_fmt NAMES fmt/format.cc)

find_path(path_include_epics_base NAMES epicsTypes.h HINTS
${epicsbase_dir}/include
)

find_library(path_lib_epics_ca NAMES ca HINTS
${epicsbase_dir}/lib/${epics_arch}
)

# The ESS EPICS v4 installation uses a non-standard schema:
# .../pv<Module>/<module-version>/<base-version>/...
# whereas a standard local EPICS compile produces
# .../pv<Module>/...
# so we support both schemes in the following.

# Of course, you can also just point the standard CMAKE_*_PATH variables to
# your custom EPICS installation.  This is what we do to test different
# EPICS versions quickly.

find_path(path_include_epics_pvData NAMES pv/pvData.h HINTS
${epicsv4_dir}/pvDataCPP/include
${epicsv4_dir}/pvDataCPP/${epics_pvData_version}/${epics_base_version}/include
)

find_library(path_lib_epics_pvData NAMES pvData pvDataCPP HINTS
${epicsv4_dir}/pvDataCPP/lib/${epics_arch}
${epicsv4_dir}/pvDataCPP/${epics_pvData_version}/${epics_base_version}/lib/${epics_arch}
)

find_path(path_include_epics_pvAccess NAMES pv/pvAccess.h HINTS
${epicsv4_dir}/pvAccessCPP/include
${epicsv4_dir}/pvAccessCPP/${epics_pvAccess_version}/${epics_base_version}/include
)

find_library(path_lib_epics_pvAccess NAMES pvAccess pvAccessCPP HINTS
${epicsv4_dir}/pvAccessCPP/lib/${epics_arch}
${epicsv4_dir}/pvAccessCPP/${epics_pvAccess_version}/${epics_base_version}/lib/${epics_arch}
)

find_path(path_include_epics_pvDatabase NAMES pv/pvDatabase.h HINTS
${epicsv4_dir}/pvDatabaseCPP/include
${epicsv4_dir}/pvDatabaseCPP/${epics_pvDatabase_version}/${epics_base_version}/include
)

find_library(path_lib_epics_pvDatabase NAMES pvDatabase pvDatabaseCPP HINTS
${epicsv4_dir}/pvDatabaseCPP/lib/${epics_arch}
${epicsv4_dir}/pvDatabaseCPP/${epics_pvDatabase_version}/${epics_base_version}/lib/${epics_arch}
)

find_path(path_include_epics_NT NAMES pv/nt.h HINTS
${epicsv4_dir}/normativeTypesCPP/include
${epicsv4_dir}/normativeTypesCPP/${epics_normativeTypes_version}/${epics_base_version}/include
)

find_library(path_lib_epics_NT NAMES nt ntCPP normativeTypesCPP HINTS
${epicsv4_dir}/normativeTypesCPP/lib/${epics_arch}
${epicsv4_dir}/normativeTypesCPP/${epics_normativeTypes_version}/${epics_base_version}/lib/${epics_arch}
)

find_path(path_include_rapidjson NAMES rapidjson/document.h)

find_path(path_include_rdkafka NAMES librdkafka/rdkafka.h)
find_library(path_lib_rdkafka NAMES rdkafka)

find_path(path_include_flatbuffers NAMES flatbuffers/flatbuffers.h)

find_path(path_include_streaming_data_types NAMES schemas/f141_epics_nt.fbs HINTS
# The common case as fallback:
${CMAKE_CURRENT_SOURCE_DIR}/../../streaming-data-types
)

find_path(path_include_pcre2 NAMES pcre2.h)
find_library(path_lib_pcre2 NAMES pcre2-8)

if (USE_GRAYLOG_LOGGER)
find_path(path_include_graylog_logger NAMES graylog_logger/Log.hpp)
find_library(path_lib_graylog_logger NAMES graylog_logger)
if (path_include_graylog_logger AND path_lib_graylog_logger)
message(STATUS "Using graylog_logger")
set(have_graylog_logger TRUE)
endif()
endif()


message(STATUS "path_include_fmt ${path_include_fmt}")
message(STATUS "path_include_epics_base ${path_include_epics_base}")
message(STATUS "path_lib_epics_ca ${path_lib_epics_ca}")
message(STATUS "path_include_epics_pvData ${path_include_epics_pvData}")
message(STATUS "path_lib_epics_pvData ${path_lib_epics_pvData}")
message(STATUS "path_include_epics_pvAccess ${path_include_epics_pvAccess}")
message(STATUS "path_lib_epics_pvAccess ${path_lib_epics_pvAccess}")
message(STATUS "path_include_epics_pvDatabase ${path_include_epics_pvDatabase}")
message(STATUS "path_lib_epics_pvDatabase ${path_lib_epics_pvDatabase}")
message(STATUS "path_include_epics_NT ${path_include_epics_NT}")
message(STATUS "path_lib_epics_NT ${path_lib_epics_NT}")
message(STATUS "path_include_rapidjson ${path_include_rapidjson}")
message(STATUS "path_include_rdkafka ${path_include_rdkafka}")
message(STATUS "path_lib_rdkafka ${path_lib_rdkafka}")
message(STATUS "path_include_flatbuffers ${path_include_flatbuffers}")
message(STATUS "path_include_streaming_data_types ${path_include_streaming_data_types}")
message(STATUS "path_include_graylog_logger ${path_include_graylog_logger}")
message(STATUS "path_lib_graylog_logger ${path_lib_graylog_logger}")



# Minimum commit of streaming-data-types that we need:
set(need_commit 0a4766575da44764ee43f2347b3d1106bdf098df)

add_custom_target(check_streaming_data_types ALL
COMMAND bash -c '\(cd ${path_include_streaming_data_types} && git merge-base --is-ancestor ${need_commit} HEAD \) || \( echo && echo ERROR\ Your\ streaming-data-types\ repository\ is\ too\ old && echo && exit 1 \) '
)

find_program(flatc flatc)
message(STATUS "flatc in ${flatc}")

set(flatbuffers_generated_headers "")

set(schemas_subdir "schemas")
set(head_out_dir "${CMAKE_CURRENT_BINARY_DIR}/${schemas_subdir}")
file(MAKE_DIRECTORY ${head_out_dir})
file(GLOB_RECURSE flatbuffers_schemata2 RELATIVE "${path_include_streaming_data_types}/schemas" "${path_include_streaming_data_types}/schemas/*.fbs")

foreach (f0 ${flatbuffers_schemata2})
	string(REGEX REPLACE "\\.fbs$" "" s0 ${f0})
	set(fbs "${s0}.fbs")
	set(fbh "${s0}_generated.h")
	add_custom_command(
		OUTPUT "${head_out_dir}/${fbh}"
		COMMAND ${flatc} --cpp --gen-mutable --gen-name-strings --scoped-enums "${path_include_streaming_data_types}/schemas/${fbs}"
		DEPENDS "${path_include_streaming_data_types}/schemas/${fbs}"
		WORKING_DIRECTORY "${head_out_dir}"
		COMMENT "Process ${fbs} using ${flatc}"
	)
	list(APPEND flatbuffers_generated_headers "${head_out_dir}/${fbh}")
endforeach()

add_custom_target(flatbuffers_generate ALL DEPENDS ${flatbuffers_generated_headers})
add_dependencies(flatbuffers_generate check_streaming_data_types)


set(xxd_inputs "schema-config-global.json")
set(xxd_generated "")
foreach (f0 ${xxd_inputs})
	set(src "${f0}")
	set(tgt "${f0}.cxx")
	add_custom_command(
		OUTPUT ${tgt}
		COMMAND xxd -i <${src} >${CMAKE_CURRENT_BINARY_DIR}/${f0}.cxx
		DEPENDS ${src}
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
		COMMENT "Process ${f0}"
	)
	list(APPEND xxd_generated "${tgt}")
endforeach()
add_custom_target(xxd_generate ALL DEPENDS ${xxd_generated})



set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -fPIC -g")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -fno-inline")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
#set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wpedantic -Wno-variadic-macros")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fomit-frame-pointer")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-inline -fno-omit-frame-pointer")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-arcs -ftest-coverage")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pg")

if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS "4.9.3")
else ()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fdiagnostics-color=auto")
endif()


# Not in EPEL on dev-env
#add_library(fmt SHARED IMPORTED)
#set_property(TARGET fmt PROPERTY IMPORTED_LOCATION ${path_lib_fmt})

add_library(epicsbase SHARED IMPORTED)
set_property(TARGET epicsbase PROPERTY IMPORTED_LOCATION ${path_lib_epics_ca})

add_library(libpvData SHARED IMPORTED)
set_property(TARGET libpvData PROPERTY IMPORTED_LOCATION ${path_lib_epics_pvData})

add_library(libpvAccess SHARED IMPORTED)
set_property(TARGET libpvAccess PROPERTY IMPORTED_LOCATION ${path_lib_epics_pvAccess})

add_library(libNormativeTypes SHARED IMPORTED)
set_property(TARGET libNormativeTypes PROPERTY IMPORTED_LOCATION ${path_lib_epics_NT})

add_library(librdkafka SHARED IMPORTED)
set_property(TARGET librdkafka PROPERTY IMPORTED_LOCATION ${path_lib_rdkafka})

add_library(libpcre2-8 SHARED IMPORTED)
set_property(TARGET libpcre2-8 PROPERTY IMPORTED_LOCATION ${path_lib_pcre2})

if (have_gtest)
add_library(gtest_static STATIC IMPORTED)
set_property(TARGET gtest_static PROPERTY IMPORTED_LOCATION "${CMAKE_CURRENT_BINARY_DIR}/../googletest/googlemock/gtest/libgtest.a")
endif()

if (have_graylog_logger)
add_library(libgraylog_logger SHARED IMPORTED)
set_property(TARGET libgraylog_logger PROPERTY IMPORTED_LOCATION ${path_lib_graylog_logger})
endif()



set(SOURCES
MainOpt.cxx
Main.cxx
ForwarderInfo.cxx
EpicsClient.cxx
helper.cxx
logger.cxx
configuration.cxx
Kafka.cxx
KafkaW.cxx
Ring.cxx
ConversionWorker.cxx
epics-to-fb.cxx
Config.cxx
fbschemas.cxx
SchemaRegistry.cxx
uri.cxx
blobs.cxx
Converter.cxx
KafkaOutput.cxx
Stream.cxx
schema_f140_general.cxx
schema_f141_epics_nt.cxx
schemas/f142/f142.cxx
${path_include_fmt}/fmt/format.cc
${CMAKE_CURRENT_BINARY_DIR}/git_commit_current.cxx
)
if (WIN32)
set(SOURCES ${SOURCES} wingetopt.c)
endif (WIN32)

set(tgt __objects)
add_library(${tgt} OBJECT ${SOURCES})
target_include_directories(${tgt} PRIVATE
${path_include_fmt}
${path_include_epics_base}
${path_include_epics_pvData}
${path_include_epics_pvAccess}
${path_include_epics_NT}
${path_include_rdkafka}
${path_include_flatbuffers}
${path_include_rapidjson}
${path_include_pcre2}
${CMAKE_CURRENT_BINARY_DIR}
"/opt/local/include/"
)
if (EXISTS "${path_include_epics_base}/os/Linux")
target_include_directories(${tgt} PRIVATE "${path_include_epics_base}/os/Linux")
endif()
if (EXISTS "${path_include_epics_base}/os/Darwin")
target_include_directories(${tgt} PRIVATE "${path_include_epics_base}/os/Darwin")
endif()
if (EXISTS "${path_include_epics_base}/compiler/gcc")
target_include_directories(${tgt} PRIVATE "${path_include_epics_base}/compiler/gcc")
endif()
if (EXISTS "${path_include_epics_base}/compiler/clang")
target_include_directories(${tgt} PRIVATE "${path_include_epics_base}/compiler/clang")
endif()
if (have_gtest)
target_compile_definitions(${tgt} PRIVATE HAVE_GTEST=1)
target_include_directories(${tgt} PRIVATE ${path_include_gtest})
endif()
add_dependencies(${tgt} flatbuffers_generate xxd_generate git_commit_current)


set(tgt forward-epics-to-kafka)
add_executable(${tgt}
forward-epics-to-kafka.cxx
$<TARGET_OBJECTS:__objects>
)
add_dependencies(${tgt} flatbuffers_generate git_commit_current)
target_include_directories(${tgt} PRIVATE
${path_include_fmt}
${path_include_flatbuffers}
${path_include_rdkafka}
${path_include_rapidjson}
${path_include_pcre2}
${path_include_epics_base}
${path_include_epics_pvData}
)
if (EXISTS "${path_include_epics_base}/os/Linux")
target_include_directories(${tgt} PRIVATE "${path_include_epics_base}/os/Linux")
endif()
if (EXISTS "${path_include_epics_base}/os/Darwin")
target_include_directories(${tgt} PRIVATE "${path_include_epics_base}/os/Darwin")
endif()
if (EXISTS "${path_include_epics_base}/compiler/gcc")
target_include_directories(${tgt} PRIVATE "${path_include_epics_base}/compiler/gcc")
endif()
if (EXISTS "${path_include_epics_base}/compiler/clang")
target_include_directories(${tgt} PRIVATE "${path_include_epics_base}/compiler/clang")
endif()

if (have_gtest)
add_dependencies(${tgt} gtest)
target_compile_definitions(${tgt} PRIVATE HAVE_GTEST=1)
target_include_directories(${tgt} PRIVATE ${path_include_gtest})
target_link_libraries(${tgt} gtest_static)
endif()

if (have_graylog_logger)
target_compile_definitions(${tgt} PRIVATE HAVE_GRAYLOG_LOGGER=1)
target_include_directories(${tgt} PRIVATE ${path_include_graylog_logger})
target_link_libraries(${tgt} libgraylog_logger)
endif()

target_link_libraries(${tgt} epicsbase)
target_link_libraries(${tgt} libpvData)
target_link_libraries(${tgt} libpvAccess)
target_link_libraries(${tgt} libNormativeTypes)
target_link_libraries(${tgt} librdkafka)
target_link_libraries(${tgt} libpcre2-8)
target_link_libraries(${tgt} pthread)

if (have_gtest)
add_subdirectory(tests)
endif()
