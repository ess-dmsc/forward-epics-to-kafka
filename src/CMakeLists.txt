message(STATUS "CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ../bin)

find_package(CURL)
find_package(Threads)
find_package(GitCommitExtract)

set(libraries_common
        CONAN_PKG::librdkafka
        CONAN_PKG::jsonformoderncpp
        CONAN_PKG::concurrentqueue
        CONAN_PKG::streaming-data-types
        CONAN_PKG::CLI11
        CONAN_PKG::fmt
        CONAN_PKG::flatbuffers
        CONAN_PKG::asio
        Threads::Threads
)

set(path_include_common "")

# Allow a system installed EPICS library to be used instead of conan package (for ISIS-STFC)
set(USE_EXTERNAL_EPICS FALSE CACHE BOOL "Look for system installed EPICS instead of using Conan package")
if (USE_EXTERNAL_EPICS)
    find_package(EPICSV4)
    list(APPEND libraries_common
            ${path_library_epics_ca}
            ${path_library_epics_pvData}
            ${path_library_epics_pvAccess}
            ${path_library_epics_NT})
    list(APPEND path_include_common ${EPICSV4_INCLUDE_DIRS})
else()
    list(APPEND libraries_common CONAN_PKG::epics)
endif(USE_EXTERNAL_EPICS)

set(compile_defs_common "")

if (CURL_FOUND)
    list(APPEND compile_defs_common "HAVE_CURL=1")
    list(APPEND libraries_common ${CURL_LIBRARIES})
    list(APPEND path_include_common ${CURL_INCLUDE_DIRS})
endif(CURL_FOUND)

set(USE_GRAYLOG_LOGGER TRUE CACHE BOOL "Compile with support for logging to Graylog")
if (CONAN_SPDLOG-GRAYLOG_ROOT AND USE_GRAYLOG_LOGGER)
    message(STATUS "Using graylog_logger")
    list(APPEND compile_defs_common "HAVE_GRAYLOG_LOGGER=1")
    list(APPEND libraries_common
            CONAN_PKG::spdlog-graylog)
else()
    list(APPEND libraries_common
            CONAN_PKG::spdlog)
endif(CONAN_SPDLOG-GRAYLOG_ROOT AND USE_GRAYLOG_LOGGER)

set(forwarder-library_INC
    EpicsClient/EpicsClientMonitor.h
    EpicsClient/EpicsClientMonitorImpl.h
    EpicsClient/EpicsClientRandom.h
    EpicsClient/EpicsClientFactory.h
    EpicsClient/EpicsClientInterface.h
    EpicsClient/FwdMonitorRequester.h
    EpicsClient/EpicsClientInterface.h
    EpicsClient/ChannelRequester.h
    KafkaW/ConsumerMessage.h
    KafkaW/KafkaW.h
    KafkaW/Producer.h
    KafkaW/ProducerTopic.h
    KafkaW/Consumer.h
    KafkaW/BrokerSettings.h
    Config.h
    ConfigParser.h
    ConversionWorker.h
    Converter.h
    CommandHandler.h
    CURLReporter.h
    EpicsPVUpdate.h
    FlatbufferMessage.h
    FlatbufferMessageSlice.h
    Forwarder.h
    FlatBufferCreator.h
    git_commit_current.h
        json.h
    Kafka.h
    KafkaW/BrokerSettings.h
    KafkaW/Consumer.h
    KafkaW/ConsumerMessage.h
    KafkaW/KafkaW.h
    KafkaW/Producer.h
    KafkaW/ProducerMessage.h
    KafkaW/ProducerDeliveryCb.h
    KafkaW/ProducerStats.h
    KafkaW/ProducerTopic.h
    KafkaW/KafkaEventCb.h
    KafkaW/MetadataException.h
    KafkaOutput.h
    logger.h
    MainOpt.h
    RangeSet.h
    SchemaRegistry.h
    Stream.h
    Streams.h
    Timer.h
    URI.h
    schemas/tdc_time/TdcTime.h
    MetricsTimer.h)

set(forwarder-library_SRC
    MainOpt.cpp
    Forwarder.cpp
    ConfigParser.cpp
    CommandHandler.cpp
    EpicsClient/EpicsClientMonitor.cpp
    EpicsClient/EpicsClientRandom.cpp
    EpicsClient/FwdMonitorRequester.cpp
    EpicsClient/EpicsClientFactory.cpp
    EpicsClient/ChannelRequester.cpp
    EpicsClient/EpicsClientInterface.cpp
    logger.cpp
    Kafka.cpp
    KafkaW/BrokerSettings.cpp
    KafkaW/Consumer.cpp
    KafkaW/Producer.cpp
    KafkaW/ProducerTopic.cpp
    ConversionWorker.cpp
    Config.cpp
    FlatbufferMessage.cpp
    SchemaRegistry.cpp
    FlatBufferCreator.cpp
    URI.cpp
    json.cpp
    Converter.cpp
    KafkaOutput.cpp
    Stream.cpp
    Streams.cpp
    schemas/f142/f142.cpp
    Timer.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/git_commit_current.cpp
    schemas/tdc_time/TdcTime.cpp
    MetricsTimer.cpp)

add_library(forwarder-library ${forwarder-library_SRC} ${forwarder-library_INC} MetricsTimer.h MetricsTimer.cpp)
target_compile_definitions(forwarder-library PRIVATE ${compile_defs_common})
target_link_libraries(forwarder-library ${libraries_common})
target_include_directories(forwarder-library SYSTEM PUBLIC ${path_include_common} INTERFACE ".")
target_include_directories(forwarder-library PRIVATE ${VERSION_INCLUDE_DIR})

if (WIN32)
    target_link_libraries(forwarder-library Ws2_32.lib dbghelp.lib)
    target_compile_options(forwarder-library PRIVATE /wd4996)
    set_target_properties(git_commit_current PROPERTIES EXCLUDE_FROM_ALL TRUE)
else()
    target_compile_options(forwarder-library PRIVATE -Wall -Wextra -Werror)
    add_dependencies(forwarder-library git_commit_current)
endif(WIN32)

add_executable(forward-epics-to-kafka Main.cpp)
target_compile_definitions(forward-epics-to-kafka PRIVATE ${compile_defs_common})
target_link_libraries(forward-epics-to-kafka forwarder-library)
if (WIN32)
    target_compile_options(forward-epics-to-kafka PRIVATE /wd4996)
else()
    target_compile_options(forward-epics-to-kafka PRIVATE -Wall -Wextra -Werror)
endif(WIN32)

set(BUILD_TESTS TRUE CACHE BOOL "Build unit tests, requires googletest")
if (BUILD_TESTS)
    add_subdirectory(tests)
endif()
