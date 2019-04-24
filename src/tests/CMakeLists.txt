set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ../../tests)

set(tgt "tests")
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
    $<TARGET_OBJECTS:__objects>
        ListenerTests.cpp
        BrokerSettingsTests.cpp
    ConfStandIn.h
        ProducerDeliveryCbTests.cpp
        ConsumerTests.cpp
    MockMessage.h
    UnitTests.cpp)
add_executable(${tgt} ${sources})
add_dependencies(${tgt} flatbuffers_generate)
target_include_directories(${tgt} PRIVATE ${path_include_common})
target_link_libraries(${tgt} ${libraries_common})
add_gtest_to_target(${tgt})
