set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ../../tests)

set(tests_SRC
    URITests.cpp
    JsonTests.cpp
    ConfigParserTests.cpp
    StreamsTests.cpp
    StreamTests.cpp
    CommandHandlerTests.cpp
    EpicsClientMonitorTests.cpp
    EpicsClientRandomTests.cpp
    ProducerTests.cpp
    TimerTests.cpp
    StreamTestUtils.cpp
    RangeSetTests.cpp
    ListenerTests.cpp
    BrokerSettingsTests.cpp
    ConfStandIn.h
    ProducerDeliveryCbTests.cpp
    ConsumerTests.cpp
    MockMessage.h
    ConvertTDCTest.cpp
    GetAlarmDetailsTests.cpp
    UnitTests.cpp
    MetricsReporterTests.cpp
    MockKafkaInstanceSet.h
    StatusReporterTests.cpp
    MockProducer.h
    ReporterHelpers.h
    ReporterHelpers.cpp)
find_package(EPICS REQUIRED)
add_executable(tests ${tests_SRC})
target_include_directories(tests SYSTEM PRIVATE ${EPICS_INCLUDE_DIR})
target_link_libraries(tests
        forwarder-library
        CONAN_PKG::gtest
        CONAN_PKG::trompeloeil)

if (WIN32)
    target_compile_options(tests PRIVATE /wd4996)
else()
    target_compile_options(tests PRIVATE -Wall -Wextra -Werror)
endif(WIN32)
