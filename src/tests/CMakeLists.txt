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
