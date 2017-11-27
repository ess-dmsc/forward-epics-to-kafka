set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ../../tests)

set(tgt "tests")
set(sources
        tests.cxx
        MainOpt.cxx
        remote.cxx
        helper_tests.cxx
        uri_tests.cxx
        streams_tests.cxx
        $<TARGET_OBJECTS:__objects>
)
add_executable(${tgt} ${sources})
add_dependencies(${tgt} flatbuffers_generate)
target_include_directories(${tgt} PRIVATE ${path_include_common})
target_link_libraries(${tgt} ${libraries_common})
add_gtest_to_target(${tgt})

configure_file(test-config-valid.json ../../tests/test-config-valid.json)
configure_file(msg-add-01.json ../../tests/msg-add-01.json)
configure_file(msg-add-02.json ../../tests/msg-add-02.json)
configure_file(msg-add-03.json ../../tests/msg-add-03.json)
configure_file(msg-add-named-converter.json ../../tests/msg-add-named-converter.json)
configure_file(msg-add-different-brokers.json ../../tests/msg-add-different-brokers.json)
configure_file(config-default.json ../../tests/config-default.json)
