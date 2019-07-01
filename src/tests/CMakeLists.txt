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
    UnitTests.cpp)

add_executable(tests ${tests_SRC})
target_link_libraries(tests
        forwarder-library
        CONAN_PKG::gtest)

if (WIN32)
    target_compile_options(tests PRIVATE /wd4996)
else()
    target_compile_options(tests PRIVATE -Wall -Wextra -Werror)
endif(WIN32)
