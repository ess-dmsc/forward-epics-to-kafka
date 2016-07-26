project(forward-epics-to-kafka)

# Relative to build root
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ..)

# Different libs use different arch tags.
# We target x86-64
set(archs linux-x86_64)

# Set e.g. environment $flatc to the directory where flatc resides.
#unset(flatc CACHE)
find_program(flatc flatc PATHS "$ENV{flatc}" "$ENV{HOME}/.tools")

# find_path(path_epics_base_lib NAMES libca.so PATHS "$ENV{epicsbase_dir}/lib/linux-x86_64")


# Using 3.15.2 because the v4 modules do not show headers yet for the more recent 3.15.4
set(epics_arch centos7-x86_64)
set(epics_base_version 3.15.2)
set(epics_modules_version 5.0.2)


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
 /opt/epics/modules/pvDataCPP/${epics_modules_version}/${epics_base_version}/include
)

find_library(path_lib_epics_pvData NAMES pvData PATHS
 "$ENV{epicsv4_dir}/pvDataCPP/lib/linux-x86_64"
 /opt/epics/modules/pvDataCPP/${epics_modules_version}/${epics_base_version}/lib/${epics_arch}
)

find_path(path_include_epics_pvAccess NAMES pv/pvAccess.h PATHS
 "$ENV{epicsv4_dir}/pvAccessCPP/include"
 /opt/epics/modules/pvAccessCPP/${epics_modules_version}/${epics_base_version}/include
)

find_library(path_lib_epics_pvAccess NAMES pvAccess PATHS
 "$ENV{epicsv4_dir}/pvAccessCPP/lib/linux-x86_64"
 /opt/epics/modules/pvAccessCPP/${epics_modules_version}/${epics_base_version}/lib/${epics_arch}
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

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 -std=c++11 -Wall -fPIC")

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-arcs -ftest-coverage")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pg")

if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS "4.9.3")
else ()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fdiagnostics-color=auto")
endif()

set(librdkafka_dir   $ENV{librdkafka_dir})
set(jansson_dir      $ENV{jansson_dir})

add_library(epicsbase SHARED IMPORTED)
set_property(TARGET epicsbase PROPERTY IMPORTED_LOCATION ${path_lib_epics_ca})

add_library(libpvData SHARED IMPORTED)
set_property(TARGET libpvData PROPERTY IMPORTED_LOCATION ${path_lib_epics_pvData})

add_library(libpvAccess SHARED IMPORTED)
set_property(TARGET libpvAccess PROPERTY IMPORTED_LOCATION ${path_lib_epics_pvAccess})

add_library(librdkafka SHARED IMPORTED)
set_property(TARGET librdkafka PROPERTY IMPORTED_LOCATION "${librdkafka_dir}/lib/librdkafka.so")

add_library(jansson SHARED IMPORTED)
set_property(TARGET jansson PROPERTY IMPORTED_LOCATION "${jansson_dir}/lib/libjansson.so")






add_executable(forward-epics-to-kafka
logger.cxx
configuration.cxx
TopicMapping.cxx
Kafka.cxx
epics.cxx
Config.cxx
main.cxx
)

set_property(TARGET forward-epics-to-kafka APPEND PROPERTY INCLUDE_DIRECTORIES ${path_include_epics_base})
set_property(TARGET forward-epics-to-kafka APPEND PROPERTY INCLUDE_DIRECTORIES ${path_include_epics_base}/os/Linux)
set_property(TARGET forward-epics-to-kafka APPEND PROPERTY INCLUDE_DIRECTORIES ${path_include_epics_base}/compiler/gcc)
set_property(TARGET forward-epics-to-kafka APPEND PROPERTY INCLUDE_DIRECTORIES ${path_include_epics_pvData})
set_property(TARGET forward-epics-to-kafka APPEND PROPERTY INCLUDE_DIRECTORIES ${path_include_epics_pvAccess})
#set_property(TARGET forward-epics-to-kafka APPEND PROPERTY INCLUDE_DIRECTORIES "${epicsv4_dir}/pvDataCPP/include")
#set_property(TARGET forward-epics-to-kafka APPEND PROPERTY INCLUDE_DIRECTORIES "${epicsv4_dir}/pvAccessCPP/include")
#set_property(TARGET forward-epics-to-kafka APPEND PROPERTY INCLUDE_DIRECTORIES "${epicsv4_dir}/pvDatabaseCPP/include")
set_property(TARGET forward-epics-to-kafka APPEND PROPERTY INCLUDE_DIRECTORIES "${librdkafka_dir}/include")
set_property(TARGET forward-epics-to-kafka APPEND PROPERTY INCLUDE_DIRECTORIES "${jansson_dir}/include")

add_dependencies(forward-epics-to-kafka flatbuffers_generate)

target_link_libraries(forward-epics-to-kafka epicsbase)
target_link_libraries(forward-epics-to-kafka libpvData)
target_link_libraries(forward-epics-to-kafka libpvAccess)
target_link_libraries(forward-epics-to-kafka librdkafka)
target_link_libraries(forward-epics-to-kafka jansson)
