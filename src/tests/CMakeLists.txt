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
<<<<<<< HEAD
    ProducerDeliveryCb_tests.cpp
    Consumer_tests.cpp
    MockMessage.h
    TdcTime_Convert.cpp
)
add_executable(${tgt} ${sources})
add_dependencies(${tgt} flatbuffers_generate)
target_include_directories(${tgt} PRIVATE ${path_include_common})
target_link_libraries(${tgt} ${libraries_common})
add_gtest_to_target(${tgt})
=======
    ProducerDeliveryCbTests.cpp
    ConsumerTests.cpp
    MockMessage.h
    UnitTests.cpp)

add_executable(tests ${tests_SRC})
target_link_libraries(tests
        forwarder-library
        CONAN_PKG::gtest)

if (WIN32)
    target_compile_options(tests PRIVATE /wd4996)
else()
    target_compile_options(tests PRIVATE -Wall -Wextra)
endif(WIN32)
>>>>>>> master
