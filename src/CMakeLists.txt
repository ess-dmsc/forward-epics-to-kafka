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


# Allows for easier testing with multiple versions on local hosts:
if (EXISTS $ENV{EPICSV4})
    set(epicsv4_dir        $ENV{EPICSV4})
else()
    set(epicsv4_dir        $ENV{epicsv4_dir})
endif()

set(epicsbase_dir      $ENV{EPICS_BASE})
set(epics_arch         $ENV{EPICS_HOST_ARCH})

string(REGEX REPLACE ".+/base-([.0-9]+)$" "\\1" epics_base_version $ENV{EPICS_BASE})
#message(FATAL_ERROR "[${epics_base_version}]")
#set(epics_base_version $ENV{EPICS_V4_BASE_VERSION})

set(epics_pvData_version 5.0.2 CACHE STRING "pvData version")
set(epics_pvAccess_version 4.1.2 CACHE STRING "pvAccess version")
set(epics_pvDatabase_version 4.1.1 CACHE STRING "pvDatabase version")
set(epics_normativeTypes_version 5.0.2 CACHE STRING "normativeTypes version")
set(USE_GRAYLOG_LOGGER TRUE CACHE BOOL "Try to use graylog_logger")

# fmt not in the EPEL pinned by the dev-env crew.
# So use the source version:
find_path(path_include_fmt NAMES fmt/format.cc PATHS
$ENV{fmt_dir}/include
/opt/local/fmt
# For DMSC build node:
$ENV{DM_ROOT}/usr/include
)

find_path(path_include_epics_base NAMES epicsTypes.h PATHS
${epicsbase_dir}/include
/opt/epics/bases/base-${epics_base_version}/include
)

find_library(path_lib_epics_ca NAMES ca PATHS
${epicsbase_dir}/lib/${epics_arch}
/opt/epics/bases/base-${epics_base_version}/lib/${epics_arch}
)

find_path(path_include_epics_pvData NAMES pv/pvData.h PATHS
${epicsv4_dir}/pvDataCPP/include
/opt/epics/modules/pvDataCPP/${epics_pvData_version}/${epics_base_version}/include
)

find_library(path_lib_epics_pvData NAMES pvData pvDataCPP PATHS
${epicsv4_dir}/pvDataCPP/lib/${epics_arch}
 /opt/epics/modules/pvDataCPP/${epics_pvData_version}/${epics_base_version}/lib/${epics_arch}
)

find_path(path_include_epics_pvAccess NAMES pv/pvAccess.h PATHS
${epicsv4_dir}/pvAccessCPP/include
/opt/epics/modules/pvAccessCPP/${epics_pvAccess_version}/${epics_base_version}/include
)

find_library(path_lib_epics_pvAccess NAMES pvAccess pvAccessCPP PATHS
${epicsv4_dir}/pvAccessCPP/lib/${epics_arch}
/opt/epics/modules/pvAccessCPP/${epics_pvAccess_version}/${epics_base_version}/lib/${epics_arch}
)

find_path(path_include_epics_pvDatabase NAMES pv/pvDatabase.h PATHS
${epicsv4_dir}/pvDatabaseCPP/include
/opt/epics/modules/pvDatabaseCPP/${epics_pvDatabase_version}/${epics_base_version}/include
)

find_library(path_lib_epics_pvDatabase NAMES pvDatabase pvDatabaseCPP PATHS
${epicsv4_dir}/pvDatabaseCPP/lib/${epics_arch}
/opt/epics/modules/pvDatabaseCPP/${epics_pvDatabase_version}/${epics_base_version}/lib/${epics_arch}
)

find_path(path_include_epics_NT NAMES pv/nt.h PATHS
${epicsv4_dir}/normativeTypesCPP/include
/opt/epics/modules/normativeTypesCPP/${epics_normativeTypes_version}/${epics_base_version}/include
)

find_library(path_lib_epics_NT NAMES nt ntCPP normativeTypesCPP PATHS
${epicsv4_dir}/normativeTypesCPP/lib/${epics_arch}
/opt/epics/modules/normativeTypesCPP/${epics_normativeTypes_version}/${epics_base_version}/lib/${epics_arch}
)

find_path(path_include_rapidjson NAMES rapidjson/document.h PATHS
$ENV{rapidjson_dir}/include
/opt/local/rapidjson.git/include
# For DMSC build node:
$ENV{DM_ROOT}/usr/include
)

find_path(path_include_rdkafka NAMES librdkafka/rdkafka.h PATHS
$ENV{librdkafka_dir}/include
/opt/local/rdkafka/install/include
# For the 'master' on DMSC build node, and another machine at PSI:
/usr/local/include
)

find_library(path_lib_rdkafka NAMES rdkafka PATHS
$ENV{librdkafka_dir}/lib
/opt/local/rdkafka/install/lib
# For DMSC build node and a machine at PSI:
/usr/local/lib
)

find_path(path_include_flatbuffers NAMES flatbuffers/flatbuffers.h PATHS
$ENV{flatbuffers_dir}/include
/opt/local/flatbuffers.git/include
# For DMSC build node:
$ENV{DM_ROOT}/usr/include
)

find_path(path_include_streaming_data_types NAMES schemas/f141_epics_nt.fbs PATHS
$ENV{streaming_data_types_dir}
${CMAKE_CURRENT_SOURCE_DIR}/../../streaming-data-types
)

if (USE_GRAYLOG_LOGGER)
find_path(path_include_graylog_logger NAMES graylog_logger/Log.hpp PATHS
$ENV{graylog_logger_dir}/include
/opt/local/graylog_logger-install
)
find_library(path_lib_graylog_logger NAMES graylog_logger PATHS
$ENV{graylog_logger_dir}/lib
/opt/local/graylog_logger-install
)
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


set(flatbuffers_generated_headers "")

# Search the dev env standard locations, but it will also look in PATH
find_program(flatc flatc PATHS "$ENV{flatc}" "$ENV{flatc_dir}" "/opt/local/flatbuffers")
message(STATUS "flatc in ${flatc}")

set(schemas_subdir "schemas")
set(head_out_dir "${CMAKE_CURRENT_BINARY_DIR}/${schemas_subdir}")
file(MAKE_DIRECTORY ${head_out_dir})
file(GLOB_RECURSE flatbuffers_schemata2 RELATIVE "${path_include_streaming_data_types}/schemas" "${path_include_streaming_data_types}/schemas/*.fbs")

foreach (f0 ${flatbuffers_schemata2})
	string(REGEX REPLACE "\\.fbs$" "" s0 ${f0})
	#message(STATUS "s0: ${s0}")
	set(fbs "${s0}.fbs")
	set(fbh "${s0}_generated.h")
	#message(STATUS "fbs: ${fbs}")
	#message(STATUS "fbh: ${fbh}")
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
${path_include_epics_base}/os/Linux
${path_include_epics_base}/os/Darwin
${path_include_epics_base}/compiler/gcc
${path_include_epics_base}/compiler/clang
${path_include_epics_pvData}
${path_include_epics_pvAccess}
${path_include_epics_NT}
${path_include_rdkafka}
${path_include_flatbuffers}
${path_include_rapidjson}
${CMAKE_CURRENT_BINARY_DIR}
"/opt/local/include/"
)
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
${path_include_epics_base}
${path_include_epics_pvData}
${path_include_epics_base}/os/Linux
${path_include_epics_base}/os/Darwin
${path_include_epics_base}/compiler/gcc
${path_include_epics_base}/compiler/clang
"/opt/local/include/"
)

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
target_link_libraries(${tgt} pcre2-8)
target_link_libraries(${tgt} pthread)

if (have_gtest)
add_subdirectory(tests)
endif()
