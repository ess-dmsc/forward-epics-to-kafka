set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ../../tests)

set(tgt "tests")
set(sources
tests.cxx
MainOpt.cxx
remote.cxx
$<TARGET_OBJECTS:__objects>
)
add_executable(${tgt} ${sources})
add_dependencies(${tgt} flatbuffers_generate)
target_include_directories(${tgt} PRIVATE
${path_include_fmt}
${path_include_rdkafka}
${path_include_flatbuffers}
${path_include_rapidjson}
${path_include_epics_base}
${path_include_epics_base}/os/Linux
${path_include_epics_base}/os/Darwin
${path_include_epics_base}/compiler/gcc
${path_include_epics_base}/compiler/clang
${path_include_epics_pvData}
${path_include_epics_pvAccess}
${path_include_epics_NT}
${CMAKE_CURRENT_BINARY_DIR}
)
target_link_libraries(${tgt} librdkafka)
target_link_libraries(${tgt} pcre2-8)
target_link_libraries(${tgt} epicsbase)
target_link_libraries(${tgt} libpvData)
target_link_libraries(${tgt} libpvAccess)
target_link_libraries(${tgt} libNormativeTypes)
if (have_gtest)
target_compile_definitions(${tgt} PRIVATE HAVE_GTEST=1)
target_include_directories(${tgt} PRIVATE ${path_include_gtest})
target_link_libraries(${tgt} gtest)
endif()

configure_file(test-config-valid.json ../../tests/test-config-valid.json)
configure_file(msg-add-01.json ../../tests/msg-add-01.json)
configure_file(msg-add-02.json ../../tests/msg-add-02.json)
configure_file(msg-add-03.json ../../tests/msg-add-03.json)
configure_file(msg-add-named-converter.json ../../tests/msg-add-named-converter.json)
configure_file(config-default.json ../../tests/config-default.json)
