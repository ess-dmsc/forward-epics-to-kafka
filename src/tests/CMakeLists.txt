set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ../../tests)

set(tgt "tests")
set(sources
    URI_tests.cpp
    json_tests.cpp
    ConfigParser_tests.cpp
    Streams_tests.cpp
    Stream_tests.cpp
    CommandHandler_tests.cpp
    EpicsClientMonitor_tests.cpp
    EpicsClientRandom_tests.cpp
    Producer_tests.cpp
    Timer_tests.cpp
    StreamTestUtils.cpp
    RangeSetTests.cpp
    $<TARGET_OBJECTS:__objects>
    Listener_tests.cpp
    BrokerSettings_tests.cpp
    ConfStandIn.h
    ProducerDeliveryCb_tests.cpp
    Consumer_tests.cpp
    MockMessage.h)
add_executable(${tgt} ${sources})
add_dependencies(${tgt} flatbuffers_generate)
target_include_directories(${tgt} PRIVATE ${path_include_common})
target_link_libraries(${tgt} ${libraries_common})
add_gtest_to_target(${tgt})
