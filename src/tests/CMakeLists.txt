set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)

set(tgt "tests")
set(test_SRC
tests.cxx
MainOpt.cxx
remote.cxx
helper.cxx
uri.cxx
CommonConversionTestFunctions.cpp
FSD_NTNDArrayTest.cpp
FSD_NTScalarArrayTest.cpp
)

set(test_INC
    tests.h
    ../MainOpt.h
    CommonConversionTestFunctions.h
    ../schemas/fsdc/fsdc_FastSamplingData_generated.h
    ../schemas/fsdc/FSD_Converter.h
)

add_executable(${tgt} ${test_SRC} ${test_INC} $<TARGET_OBJECTS:__objects>)
add_dependencies(${tgt} flatbuffers_generate)
target_include_directories(${tgt} PRIVATE ${path_include_common} ${GTEST_INCLUDE_DIRS})
target_link_libraries(${tgt} ${libraries_common} ${GTEST_BOTH_LIBRARIES})

configure_file(test-config-valid.json ../../tests/test-config-valid.json)
configure_file(msg-add-01.json ../../tests/msg-add-01.json)
configure_file(msg-add-02.json ../../tests/msg-add-02.json)
configure_file(msg-add-03.json ../../tests/msg-add-03.json)
configure_file(msg-add-named-converter.json ../../tests/msg-add-named-converter.json)
configure_file(msg-add-different-brokers.json ../../tests/msg-add-different-brokers.json)
configure_file(config-default.json ../../tests/config-default.json)
