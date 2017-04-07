# According to the official ESS wiki documentation, the base path to the
# EPICS v4 modules is given by EPICS_MODULES_PATH
# Also, all EPICS bases can be found in EPICS_BASES_PATH

# Even though not mentioned on the wiki, the official ESS EPICS installation
# provides the version of the EPICS base as used by EPICS v4
# in EPICS_V4_BASE_VERSION

set(epics_base_version "$ENV{EPICS_V4_BASE_VERSION}")
set(epicsbase_dir      ".")
if (DEFINED ENV{EPICS_BASES_PATH})
set(epicsbase_dir      "$ENV{EPICS_BASES_PATH}/base-${epics_base_version}")
endif()
set(epics_arch         "$ENV{EPICS_HOST_ARCH}")
set(epicsv4_dir        ".")
if (DEFINED ENV{EPICS_MODULES_PATH})
set(epicsv4_dir        "$ENV{EPICS_MODULES_PATH}")
endif()

# Currently, the official environment gives no hint about which specific
# version of the modules we should use.  We therefore pin it to the ones
# which are currently considered as 'production' for our purposes.

set(epics_pvData_version 5.0.2 CACHE STRING "pvData version")
set(epics_pvAccess_version 4.1.2 CACHE STRING "pvAccess version")
set(epics_pvDatabase_version 4.1.1 CACHE STRING "pvDatabase version")
set(epics_normativeTypes_version 5.0.2 CACHE STRING "normativeTypes version")

find_path(path_include_epics_base NAMES epicsTypes.h HINTS
${epicsbase_dir}/include
)

find_library(path_library_epics_ca NAMES ca HINTS
${epicsbase_dir}/lib/${epics_arch}
)

# The ESS EPICS v4 installation uses a non-standard schema:
# .../pv<Module>/<module-version>/<base-version>/...
# whereas a standard local EPICS compile produces
# .../pv<Module>/...
# so we support both schemes in the following.

# Of course, you can also just point the standard CMAKE_*_PATH variables to
# your custom EPICS installation.  This is what we do to test different
# EPICS versions quickly.

find_path(path_include_epics_pvData NAMES pv/pvData.h HINTS
${epicsv4_dir}/pvDataCPP/include
${epicsv4_dir}/pvDataCPP/${epics_pvData_version}/${epics_base_version}/include
)

find_library(path_library_epics_pvData NAMES pvData pvDataCPP HINTS
${epicsv4_dir}/pvDataCPP/lib/${epics_arch}
${epicsv4_dir}/pvDataCPP/${epics_pvData_version}/${epics_base_version}/lib/${epics_arch}
)

find_path(path_include_epics_pvAccess NAMES pv/pvAccess.h HINTS
${epicsv4_dir}/pvAccessCPP/include
${epicsv4_dir}/pvAccessCPP/${epics_pvAccess_version}/${epics_base_version}/include
)

find_library(path_library_epics_pvAccess NAMES pvAccess pvAccessCPP HINTS
${epicsv4_dir}/pvAccessCPP/lib/${epics_arch}
${epicsv4_dir}/pvAccessCPP/${epics_pvAccess_version}/${epics_base_version}/lib/${epics_arch}
)

find_path(path_include_epics_pvDatabase NAMES pv/pvDatabase.h HINTS
${epicsv4_dir}/pvDatabaseCPP/include
${epicsv4_dir}/pvDatabaseCPP/${epics_pvDatabase_version}/${epics_base_version}/include
)

find_library(path_library_epics_pvDatabase NAMES pvDatabase pvDatabaseCPP HINTS
${epicsv4_dir}/pvDatabaseCPP/lib/${epics_arch}
${epicsv4_dir}/pvDatabaseCPP/${epics_pvDatabase_version}/${epics_base_version}/lib/${epics_arch}
)

find_path(path_include_epics_NT NAMES pv/nt.h HINTS
${epicsv4_dir}/normativeTypesCPP/include
${epicsv4_dir}/normativeTypesCPP/${epics_normativeTypes_version}/${epics_base_version}/include
)

find_library(path_library_epics_NT NAMES nt ntCPP normativeTypesCPP HINTS
${epicsv4_dir}/normativeTypesCPP/lib/${epics_arch}
${epicsv4_dir}/normativeTypesCPP/${epics_normativeTypes_version}/${epics_base_version}/lib/${epics_arch}
)

message(STATUS "path_include_epics_base       ${path_include_epics_base}")
message(STATUS "path_library_epics_ca         ${path_library_epics_ca}")
message(STATUS "path_include_epics_pvData     ${path_include_epics_pvData}")
message(STATUS "path_library_epics_pvData     ${path_library_epics_pvData}")
message(STATUS "path_include_epics_pvAccess   ${path_include_epics_pvAccess}")
message(STATUS "path_library_epics_pvAccess   ${path_library_epics_pvAccess}")
message(STATUS "path_include_epics_pvDatabase ${path_include_epics_pvDatabase}")
message(STATUS "path_library_epics_pvDatabase ${path_library_epics_pvDatabase}")
message(STATUS "path_include_epics_NT         ${path_include_epics_NT}")
message(STATUS "path_library_epics_NT         ${path_library_epics_NT}")

set(path_include_epics_all ${path_include_epics_base} ${path_include_epics_pvData} ${path_include_epics_pvAccess} ${path_include_epics_pvDatabase} ${path_include_epics_NT})

if (EXISTS "${path_include_epics_base}/os/Linux")
list(APPEND path_include_epics_all "${path_include_epics_base}/os/Linux")
endif()
if (EXISTS "${path_include_epics_base}/os/Darwin")
list(APPEND path_include_epics_all "${path_include_epics_base}/os/Darwin")
endif()
if (EXISTS "${path_include_epics_base}/compiler/gcc")
list(APPEND path_include_epics_all "${path_include_epics_base}/compiler/gcc")
endif()
if (EXISTS "${path_include_epics_base}/compiler/clang")
list(APPEND path_include_epics_all "${path_include_epics_base}/compiler/clang")
endif()

add_library(libepicsbase SHARED IMPORTED)
set_property(TARGET libepicsbase PROPERTY IMPORTED_LOCATION ${path_library_epics_ca})

add_library(libpvData SHARED IMPORTED)
set_property(TARGET libpvData PROPERTY IMPORTED_LOCATION ${path_library_epics_pvData})

add_library(libpvAccess SHARED IMPORTED)
set_property(TARGET libpvAccess PROPERTY IMPORTED_LOCATION ${path_library_epics_pvAccess})

add_library(libNormativeTypes SHARED IMPORTED)
set_property(TARGET libNormativeTypes PROPERTY IMPORTED_LOCATION ${path_library_epics_NT})
