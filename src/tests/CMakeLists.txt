set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ../../tests)

set(sources
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
    UnitTests.cpp)
add_executable(tests ${sources})
target_include_directories(tests PRIVATE ${path_include_common})
target_link_libraries(tests
        forwarder-library
        CONAN_PKG::gtest)
if (WIN32)
    target_compile_options(tests PRIVATE /wd4996)
else()
    target_compile_options(tests PRIVATE -Wall -Wextra -Werror)
endif(WIN32)
