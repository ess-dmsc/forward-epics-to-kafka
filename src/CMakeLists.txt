set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ..)

message(STATUS "CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")

find_package(EPICSv4)
find_package(Fmt)
find_package(Rapidjson)
find_package(RdKafka)
find_package(Flatbuffers)
find_package(StreamingDataTypes COMPONENTS ce9a4745431e54bc45b0c927857a866fa77b84ec)
find_package(PCRE2)
find_package(GraylogLogger)
find_package(StaticData COMPONENTS "schema-config-global.json")
find_package(GitCommitExtract)


set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -fPIC -g")

if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS "4.9.3")
else ()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fdiagnostics-color=auto")
endif()


set(path_include_common
${path_include_fmt}
${path_include_rdkafka}
${path_include_flatbuffers}
${path_include_rapidjson}
${path_include_pcre2}
${path_include_epics_all}
${PROJECT_SOURCE_DIR}/src
${PROJECT_BINARY_DIR}/src
)

set(libraries_common
libepicsbase
libpvData
libpvAccess
libNormativeTypes
librdkafka
libpcre2-8
pthread
)





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
target_include_directories(${tgt} PRIVATE ${path_include_common})
add_dependencies(${tgt} flatbuffers_generate xxd_generate git_commit_current)



set(tgt forward-epics-to-kafka)
add_executable(${tgt}
forward-epics-to-kafka.cxx
$<TARGET_OBJECTS:__objects>
)
add_dependencies(${tgt} flatbuffers_generate git_commit_current)
target_include_directories(${tgt} PRIVATE ${path_include_common})

if (have_graylog_logger)
target_compile_definitions(${tgt} PRIVATE HAVE_GRAYLOG_LOGGER=1)
target_include_directories(${tgt} PRIVATE ${path_include_graylog_logger})
target_link_libraries(${tgt} libgraylog_logger)
endif()

target_link_libraries(${tgt} ${libraries_common})



if (have_gtest)
add_subdirectory(tests)
endif()
