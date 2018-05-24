set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ..)

message(STATUS "CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")

find_package(EPICSV4)
find_package(Fmt REQUIRED)
find_package(RdKafka REQUIRED)
find_package(FlatBuffers REQUIRED)
find_package(StreamingDataTypes COMPONENTS ce9a4745431e54bc45b0c927857a866fa77b84ec)
find_package(CURL)
find_package(GraylogLogger)
find_package(StaticData COMPONENTS "schema-config-global.json")
find_package(GitCommitExtract)


set(path_include_common
${FMT_INCLUDE_DIR}
${RDKAFKA_INCLUDE_DIR}
${FLATBUFFERS_INCLUDE_DIR}
${CURL_INCLUDE_DIRS}
${PROJECT_SOURCE_DIR}/src
${PROJECT_BINARY_DIR}/src
)

set(path_include_common_suppressed_warnings
${EPICSV4_INCLUDE_DIRS}
)

set(libraries_common
${path_library_epics_ca}
${path_library_epics_pvaClient}
${path_library_epics_pvData}
${path_library_epics_pvAccess}
${path_library_epics_NT}
${RDKAFKA_LIBRARIES}
${CURL_LIBRARIES}
)

if (UNIX)
  list(APPEND libraries_common pthread)
endif()

set(compile_defs_common "")

if (CURL_FOUND)
	list(APPEND compile_defs_common "HAVE_CURL=1")
endif()

set(INCLUDES
    EpicsClient/ChannelRequester.h
    EpicsClient/EpicsClientMonitor.h
    EpicsClient/EpicsClientFactory.h
    EpicsClient/EpicsClientPeriodic.h
    EpicsClient/FwdMonitorRequester.h
    EpicsClient/EpicsClientInterface.h
    Config.h
    ConversionWorker.h
    Converter.h
    CURLReporter.h
    EpicsPVUpdate.h
    FlatbufferMessage.h
    FlatbufferMessageSlice.h
    MakeFlatBufferFromPVStructure.h
    git_commit_current.h
    helper.h
    json.h
    Kafka.h
    KafkaOutput.h
    logger.h
    Main.h
    MainOpt.h
    RangeSet.h
    Ring.h
    SchemaRegistry.h
    Stream.h
    Streams.h
    uri.h
    PeriodicPVPoller.h
)

set(SOURCES
    MainOpt.cpp
    Main.cpp
    EpicsClient/EpicsClientMonitor.cpp
    EpicsClient/ChannelRequester.cpp
    EpicsClient/FwdMonitorRequester.cpp
    EpicsClient/EpicsClientFactory.cpp
    EpicsClient/EpicsClientPeriodic.cpp
    helper.cpp
    logger.cpp
    Kafka.cpp
    KafkaW/BrokerSettings.cpp
    KafkaW/Consumer.cpp
    KafkaW/KafkaW.cpp
    KafkaW/Msg.cpp
    KafkaW/PollStatus.cpp
    KafkaW/Producer.cpp
    KafkaW/ProducerTopic.cpp
    KafkaW/TopicSettings.cpp
    Ring.cpp
    ConversionWorker.cpp
    Config.cpp
    FlatbufferMessage.cpp
    SchemaRegistry.cpp
    MakeFlatBufferFromPVStructure.cpp
    uri.cpp
    json.cpp
    Converter.cpp
    KafkaOutput.cpp
    Stream.cpp
    Streams.cpp
    schema_f140_general.cpp
    schema_f141_epics_nt.cpp
    schemas/f142/f142.cpp
    ${FMT_SRC}
    schemas/f143/f143.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/git_commit_current.cpp
)

set(tgt __objects)
add_library(${tgt} OBJECT ${SOURCES} ${INCLUDES})
target_include_directories(${tgt} PRIVATE ${path_include_common})
target_include_directories(${tgt} SYSTEM PRIVATE ${path_include_common_suppressed_warnings})

if (WIN32)
    # Skip xxd_generate and git_commit_current
    add_dependencies(${tgt} flatbuffers_generate)
    set_target_properties(xxd_generate PROPERTIES EXCLUDE_FROM_ALL TRUE)
    set_target_properties(git_commit_current PROPERTIES EXCLUDE_FROM_ALL TRUE)
else()
    add_dependencies(${tgt} flatbuffers_generate xxd_generate git_commit_current)
endif(WIN32)

target_compile_definitions(${tgt} PRIVATE ${compile_defs_common})

set(tgt forward-epics-to-kafka)
add_executable(${tgt}
forward-epics-to-kafka.cpp
$<TARGET_OBJECTS:__objects>
)

if (WIN32)
    # Skip git_commit_current
    add_dependencies(${tgt} flatbuffers_generate)
else()
    add_dependencies(${tgt} flatbuffers_generate git_commit_current)
endif(WIN32)

target_compile_definitions(${tgt} PRIVATE ${compile_defs_common})
target_include_directories(${tgt} PRIVATE ${path_include_common})

if (WIN32)
    target_link_libraries(${tgt} Ws2_32.lib)
endif(WIN32)

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
