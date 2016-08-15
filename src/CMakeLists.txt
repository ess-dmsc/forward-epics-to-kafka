project(forward-epics-to-kafka)

# Relative to build root
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ..)

if (NOT CMAKE_BUILD_TYPE)
set(CMAKE_BUILD_TYPE Release)
endif()
message("Build type used: ${CMAKE_BUILD_TYPE}")


file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/git_commit_current.cxx "extern \"C\" char const GIT_COMMIT[] = \"NOTSET\";\n")

add_custom_target(git_commit_now
COMMAND echo 'extern \"C\" char const GIT_COMMIT[] = \"'`git rev-parse HEAD`'\"\;' > ${CMAKE_CURRENT_BINARY_DIR}/git_commit_now
WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_custom_target(git_commit_current
COMMAND git diff --name-only --no-index ${CMAKE_CURRENT_BINARY_DIR}/git_commit_now ${CMAKE_CURRENT_BINARY_DIR}/git_commit_current.cxx || cp ${CMAKE_CURRENT_BINARY_DIR}/git_commit_now ${CMAKE_CURRENT_BINARY_DIR}/git_commit_current.cxx
DEPENDS git_commit_now
)

# Different libs use different arch tags.
# We target x86-64
set(archs linux-x86_64)

# Set e.g. environment $flatc to the directory where flatc resides.
#unset(flatc CACHE)
find_program(flatc flatc PATHS "$ENV{flatc}" "$ENV{HOME}/.tools" "/opt/local/flatbuffers")

# find_path(path_epics_base_lib NAMES libca.so PATHS "$ENV{epicsbase_dir}/lib/linux-x86_64")


# Using 3.15.2 because the v4 modules do not show headers yet for the more recent 3.15.4
set(epics_arch centos7-x86_64)
set(epics_base_version 3.15.2)

# TODO
# Should these versions be pinned here in the cmake file, or should we allow wildcard?
set(epics_pvData_version   5.0.2)
set(epics_pvAccess_version 4.1.2)


# os/Linux/epicsMath.h
# compiler/gcc/compilerSpecific.h


find_path(path_include_epics_base NAMES epicsTypes.h PATHS
 "$ENV{epicsbase_dir}/include"
 /opt/epics/bases/base-${epics_base_version}/include
)

find_library(path_lib_epics_ca NAMES ca PATHS
 $ENV{epicsbase_dir}/lib/linux-x86_64
 /opt/epics/bases/base-${epics_base_version}/lib/${epics_arch}
)

find_path(path_include_epics_pvData NAMES pv/pvData.h PATHS
 "$ENV{epicsv4_dir}/pvDataCPP/include"
 /opt/epics/modules/pvDataCPP/${epics_pvData_version}/${epics_base_version}/include
)

find_library(path_lib_epics_pvData NAMES pvData pvDataCPP PATHS
 "$ENV{epicsv4_dir}/pvDataCPP/lib/linux-x86_64"
 /opt/epics/modules/pvDataCPP/${epics_pvData_version}/${epics_base_version}/lib/${epics_arch}
)

find_path(path_include_epics_pvAccess NAMES pv/pvAccess.h PATHS
 "$ENV{epicsv4_dir}/pvAccessCPP/include"
 /opt/epics/modules/pvAccessCPP/${epics_pvAccess_version}/${epics_base_version}/include
)

find_library(path_lib_epics_pvAccess NAMES pvAccess pvAccessCPP PATHS
 "$ENV{epicsv4_dir}/pvAccessCPP/lib/linux-x86_64"
 /opt/epics/modules/pvAccessCPP/${epics_pvAccess_version}/${epics_base_version}/lib/${epics_arch}
)

find_path(path_include_jansson NAMES jansson.h PATHS
 /usr/include
 "$ENV{jansson_dir}/include"
 /opt/local/jansson-2.7-install/include
)

find_library(path_lib_jansson NAMES jansson PATHS
 /usr/lib64
 "$ENV{jansson_dir}/lib"
 /opt/local/jansson-2.7-install/lib
)

find_path(path_include_rdkafka NAMES librdkafka/rdkafka.h PATHS
 "$ENV{librdkafka_dir}/include"
 /opt/local/rdkafka/install/include
)

find_library(path_lib_rdkafka NAMES rdkafka PATHS
 "$ENV{librdkafka_dir}/lib"
 /opt/local/rdkafka/install/lib
)

find_path(path_include_flatbuffers NAMES flatbuffers/flatbuffers.h PATHS
 "$ENV{flatbuffers_dir}/include"
 /opt/local/flatbuffers.git/include
)


set(cmake_include_current_dir on)

set(flatbuffers_schemata "simple")
set(flatbuffers_generated_headers "")

foreach (f ${flatbuffers_schemata})
	set(fbs "${f}.fbs")
	set(fbh "${f}_generated.h")
	add_custom_command(
		OUTPUT ${fbh}
		COMMAND ${flatc} --cpp "${CMAKE_CURRENT_SOURCE_DIR}/${fbs}"
		DEPENDS "${fbs}"
		COMMENT "Process ${fbs} using ${flatc}"
	)
	list(APPEND flatbuffers_generated_headers "${fbh}")
endforeach()

add_custom_target(flatbuffers_generate ALL DEPENDS ${flatbuffers_generated_headers})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -fPIC -g")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -fno-inline")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
#set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wpedantic -Wno-variadic-macros")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fomit-frame-pointer")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-inline -fno-omit-frame-pointer")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-arcs -ftest-coverage")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pg")

if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS "4.9.3")
else ()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fdiagnostics-color=auto")
endif()


add_library(epicsbase SHARED IMPORTED)
set_property(TARGET epicsbase PROPERTY IMPORTED_LOCATION ${path_lib_epics_ca})

add_library(libpvData SHARED IMPORTED)
set_property(TARGET libpvData PROPERTY IMPORTED_LOCATION ${path_lib_epics_pvData})

add_library(libpvAccess SHARED IMPORTED)
set_property(TARGET libpvAccess PROPERTY IMPORTED_LOCATION ${path_lib_epics_pvAccess})

add_library(librdkafka SHARED IMPORTED)
set_property(TARGET librdkafka PROPERTY IMPORTED_LOCATION ${path_lib_rdkafka})

add_library(jansson SHARED IMPORTED)
set_property(TARGET jansson PROPERTY IMPORTED_LOCATION ${path_lib_jansson})






add_executable(forward-epics-to-kafka
helper.cxx
logger.cxx
configuration.cxx
TopicMapping.cxx
Kafka.cxx
epics.cxx
Config.cxx
main.cxx
${CMAKE_CURRENT_BINARY_DIR}/git_commit_current.cxx
)
add_dependencies(forward-epics-to-kafka git_commit_current)

set_property(TARGET forward-epics-to-kafka APPEND PROPERTY INCLUDE_DIRECTORIES ${path_include_epics_base})
set_property(TARGET forward-epics-to-kafka APPEND PROPERTY INCLUDE_DIRECTORIES ${path_include_epics_base}/os/Linux)
set_property(TARGET forward-epics-to-kafka APPEND PROPERTY INCLUDE_DIRECTORIES ${path_include_epics_base}/compiler/gcc)
set_property(TARGET forward-epics-to-kafka APPEND PROPERTY INCLUDE_DIRECTORIES ${path_include_epics_pvData})
set_property(TARGET forward-epics-to-kafka APPEND PROPERTY INCLUDE_DIRECTORIES ${path_include_epics_pvAccess})
set_property(TARGET forward-epics-to-kafka APPEND PROPERTY INCLUDE_DIRECTORIES ${path_include_rdkafka})
set_property(TARGET forward-epics-to-kafka APPEND PROPERTY INCLUDE_DIRECTORIES ${path_include_jansson})
set_property(TARGET forward-epics-to-kafka APPEND PROPERTY INCLUDE_DIRECTORIES ${path_include_flatbuffers})

# Find the generated flat buffer
set_property(TARGET forward-epics-to-kafka APPEND PROPERTY INCLUDE_DIRECTORIES ${CMAKE_CURRENT_BINARY_DIR})


add_dependencies(forward-epics-to-kafka flatbuffers_generate)

target_link_libraries(forward-epics-to-kafka epicsbase)
target_link_libraries(forward-epics-to-kafka libpvData)
target_link_libraries(forward-epics-to-kafka libpvAccess)
target_link_libraries(forward-epics-to-kafka librdkafka)
target_link_libraries(forward-epics-to-kafka jansson)




add_executable(read-buffer
helper.cxx
logger.cxx
read-buffer.cxx
)
set_property(TARGET read-buffer APPEND PROPERTY INCLUDE_DIRECTORIES ${path_include_flatbuffers})
set_property(TARGET read-buffer APPEND PROPERTY INCLUDE_DIRECTORIES ${CMAKE_CURRENT_BINARY_DIR})
add_dependencies(read-buffer flatbuffers_generate)



set(tgt config-msg)
add_executable(config-msg
helper.cxx
logger.cxx
config-msg.cxx
${CMAKE_CURRENT_BINARY_DIR}/git_commit_current.cxx
)
add_dependencies(forward-epics-to-kafka git_commit_current)
set_property(TARGET ${tgt} APPEND PROPERTY INCLUDE_DIRECTORIES ${path_include_jansson})
set_property(TARGET ${tgt} APPEND PROPERTY INCLUDE_DIRECTORIES ${path_include_rdkafka})
target_link_libraries(${tgt} jansson)
target_link_libraries(${tgt} librdkafka)



#set(files-copy "")
#list(APPEND files-copy test-adding-removing.rb)

set(x test-adding-removing.rb)
add_custom_command(
OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${x}
COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/${x} ${CMAKE_CURRENT_BINARY_DIR}/${x}
DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${x})

add_custom_target(copy_files ALL DEPENDS
"${CMAKE_CURRENT_BINARY_DIR}/test-adding-removing.rb"
)
