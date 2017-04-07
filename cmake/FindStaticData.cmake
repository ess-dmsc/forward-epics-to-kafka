# You can pass a list of COMPONENTS when you find_package() this package
# and it will convert the listed files into blobs to be included in your
# project.
# Make your target depend on 'xxd_generate'.
# Contact: Dominik Werder

set(xxd_generated "")
foreach (f0 ${StaticData_FIND_COMPONENTS})
	set(src "${f0}")
	set(tgt "${f0}.cxx")
	add_custom_command(
		OUTPUT ${tgt}
		COMMAND xxd -i <${src} >${CMAKE_CURRENT_BINARY_DIR}/${f0}.cxx
		DEPENDS ${src}
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
		COMMENT "Process ${f0}"
	)
	list(APPEND xxd_generated "${tgt}")
endforeach()
add_custom_target(xxd_generate ALL DEPENDS ${xxd_generated})
