set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ..)

message(STATUS "CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")

find_package(EPICSV4)
find_package(Fmt REQUIRED)
find_package(Rapidjson REQUIRED)
find_package(RdKafka REQUIRED)
find_package(FlatBuffers REQUIRED)
find_package(StreamingDataTypes COMPONENTS ce9a4745431e54bc45b0c927857a866fa77b84ec)
find_package(PCREX REQUIRED)
find_package(CURL)
find_package(GraylogLogger)
find_package(StaticData COMPONENTS "schema-config-global.json")
find_package(GitCommitExtract)


set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -fPIC -g")

if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS "4.9.3")
else ()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fdiagnostics-color=auto")
endif()


set(path_include_common
${FMT_INCLUDE_DIR}
${RDKAFKA_INCLUDE_DIR}
${FLATBUFFERS_INCLUDE_DIR}
${RAPIDJSON_INCLUDE_DIR}
${CURL_INCLUDE_DIRS}
${PCREX_INCLUDE_DIRS}
${EPICSV4_INCLUDE_DIRS}
${PROJECT_SOURCE_DIR}/src
${PROJECT_BINARY_DIR}/src
)

set(libraries_common
libepicsbase
libpvData
libpvAccess
libNormativeTypes
librdkafka
${CURL_LIBRARIES}
${PCREX_LIBRARIES}
pthread
)

set(compile_defs_common ${PCREX_COMPILER_DEFINITIONS})

if (PCREX_MAJOR EQUAL 2)
	list(APPEND compile_defs_common "use_pcre2=1")
endif()

if (CURL_FOUND)
	list(APPEND compile_defs_common "HAVE_CURL=1")
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
MakeFlatBufferFromPVStructure.cxx
uri.cxx
blobs.cxx
Converter.cxx
KafkaOutput.cxx
Stream.cxx
schema_f140_general.cxx
schema_f141_epics_nt.cxx
schemas/f142/f142.cxx
${FMT_SRC}
schemas/f143/f143.cxx
${CMAKE_CURRENT_BINARY_DIR}/git_commit_current.cxx
)
if (WIN32)
set(SOURCES ${SOURCES} wingetopt.c)
endif (WIN32)

set(tgt __objects)
add_library(${tgt} OBJECT ${SOURCES})
target_include_directories(${tgt} PRIVATE ${path_include_common})
add_dependencies(${tgt} flatbuffers_generate xxd_generate git_commit_current)
target_compile_definitions(${tgt} PRIVATE ${compile_defs_common})



set(tgt forward-epics-to-kafka)
add_executable(${tgt}
forward-epics-to-kafka.cxx
$<TARGET_OBJECTS:__objects>
)
add_dependencies(${tgt} flatbuffers_generate git_commit_current)
target_compile_definitions(${tgt} PRIVATE ${compile_defs_common})
target_include_directories(${tgt} PRIVATE ${path_include_common})

if (${GRAYLOGLOGGER_FOUND} AND ${USE_GRAYLOG_LOGGER})
    message(STATUS "Using graylog_logger")
    target_compile_definitions(${tgt} PRIVATE HAVE_GRAYLOG_LOGGER=1)
    target_include_directories(${tgt} PRIVATE ${GRAYLOGLOGGER_INCLUDE_DIR})
    target_link_libraries(${tgt} libgraylog_logger)
elseif(NOT GRAYLOGLOGGER_FOUND AND USE_GRAYLOG_LOGGER)
    message(WARNING "graylog_logger requested but not found")
endif()

target_link_libraries(${tgt} ${libraries_common})



if (have_gtest)
add_subdirectory(tests)
endif()
