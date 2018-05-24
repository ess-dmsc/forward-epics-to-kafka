# According to the official ESS wiki documentation, the base path to the
# EPICS v4 modules is given by EPICS_MODULES_PATH
# Also, all EPICS bases can be found in EPICS_BASES_PATH

# Even though not mentioned on the wiki, the official ESS EPICS installation
# provides the version of the EPICS base as used by EPICS v4
# in EPICS_V4_BASE_VERSION

if (DEFINED ENV{EPICS_V4_BASE_VERSION})
	set(epics_base_version "$ENV{EPICS_V4_BASE_VERSION}")
elseif (DEFINED ENV{BASE})
	set(epics_base_version "$ENV{BASE}")
else()
	message(STATUS "Unable to determine EPICS base version.")
endif()

if (DEFINED ENV{EPICS_BASE})
	set(epicsbase_dir      "$ENV{EPICS_BASE}")
	# It is NOT an error if this variable is not set.
	# We want to be able to discover it using standard CMAKE_PATH variables too.

elseif (DEFINED ENV{EPICS_BASES_PATH})
	# Yes, we do use this possibility.  Please do not remove.
	set(x "$ENV{EPICS_BASES_PATH}/base-${epics_base_version}")
	if (EXISTS "${x}")
		set(epicsbase_dir "${x}")
	endif()
endif()

set(epics_arch "$ENV{EPICS_HOST_ARCH}")

if (DEFINED epicsv4_dir)
	message(STATUS "EPICSv4 path manually set to ${epicsv4_dir}.")
elseif (DEFINED ENV{EPICS_V4_DIR})
	set(epicsv4_dir "$ENV{EPICS_V4_DIR}")
elseif (DEFINED ENV{EPICS_MODULES_PATH})
	# NOTE
	# It is NOT a fatal error is we do not find it here.
	# find_path() can still find it if specified using CMAKE_PATH variables
	# and we do use that possibility.
	if (EXISTS "$ENV{EPICS_MODULES_PATH}/pvDataCPP/")
		set(epicsv4_dir        "$ENV{EPICS_MODULES_PATH}")
  elseif(EXISTS "$ENV{EPICS_MODULES_PATH}/epicsv4/pvDataCPP/")
    set(epicsv4_dir        "$ENV{EPICS_MODULES_PATH}/epicsv4")
  endif()
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
message("${epicsv4_dir}")
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

find_path(path_include_epics_pvaClient NAMES pv/pvaClient.h HINTS
		${epicsv4_dir}/pvaClient/include
		${epicsv4_dir}/pvaClientCPP/${epics_pvDatabase_version}/${epics_base_version}/include
		)

find_library(path_library_epics_pvaClient NAMES pvaClient pvaClientCPP HINTS
		${epicsv4_dir}/pvaClientCPP/lib/${epics_arch}
		${epicsv4_dir}/pvaClientCPP/${epics_pvDatabase_version}/${epics_base_version}/lib/${epics_arch}
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
message(STATUS "path_include_epics_pvDatabase ${path_include_epics_pvaClient}")
message(STATUS "path_library_epics_pvDatabase ${path_library_epics_pvaClient}")
message(STATUS "path_include_epics_NT         ${path_include_epics_NT}")
message(STATUS "path_library_epics_NT         ${path_library_epics_NT}")


# We could check all of them, but is it worth it?

if (NOT path_include_epics_base)
message(FATAL_ERROR "Can not find EPICS Base")
endif()

if (NOT path_include_epics_pvData)
message(FATAL_ERROR "Can not find EPICS v4")
endif()


set(EPICSV4_INCLUDE_DIRS ${path_include_epics_base} ${path_include_epics_pvData} ${path_include_epics_pvAccess} ${path_include_epics_pvDatabase} ${path_include_epics_NT})

if (EXISTS "${path_include_epics_base}/os/Linux")
list(APPEND EPICSV4_INCLUDE_DIRS "${path_include_epics_base}/os/Linux")
endif()
if (EXISTS "${path_include_epics_base}/os/Darwin")
list(APPEND EPICSV4_INCLUDE_DIRS "${path_include_epics_base}/os/Darwin")
endif()
if (EXISTS "${path_include_epics_base}/os/WIN32")
list(APPEND EPICSV4_INCLUDE_DIRS "${path_include_epics_base}/os/WIN32")
endif()
if (EXISTS "${path_include_epics_base}/compiler/msvc")
list(APPEND EPICSV4_INCLUDE_DIRS "${path_include_epics_base}/compiler/msvc")
endif()
if (EXISTS "${path_include_epics_base}/compiler/gcc")
list(APPEND EPICSV4_INCLUDE_DIRS "${path_include_epics_base}/compiler/gcc")
endif()
if (EXISTS "${path_include_epics_base}/compiler/clang")
list(APPEND EPICSV4_INCLUDE_DIRS "${path_include_epics_base}/compiler/clang")
endif()

add_library(libepicsbase SHARED IMPORTED)
set_property(TARGET libepicsbase PROPERTY IMPORTED_LOCATION ${path_library_epics_ca})

add_library(libpvData SHARED IMPORTED)
set_property(TARGET libpvData PROPERTY IMPORTED_LOCATION ${path_library_epics_pvData})

add_library(libpvAccess SHARED IMPORTED)
set_property(TARGET libpvAccess PROPERTY IMPORTED_LOCATION ${path_library_epics_pvAccess})

add_library(libNormativeTypes SHARED IMPORTED)
set_property(TARGET libNormativeTypes PROPERTY IMPORTED_LOCATION ${path_library_epics_NT})
