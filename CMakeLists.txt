# Root CMakeLists.txt

# Specify C++ 20
add_compile_options(-std=c++20)

cmake_minimum_required (VERSION 3.20)
project (Runtime
	LANGUAGES CXX
	VERSION 0.11.0
)

# Options
option(RUNTIME_BUILD_TESTS "Configure and build tests" TRUE)
option(RUNTIME_INSTALL "Run the install command when building" FALSE)
# Whether or not to build debug code. If you think this way of doing it sucks, I agree, but CMake
# is annoying and I can't get cmakedefine to work
set(RUNTIME_DEBUG 0)

# Output
if(NOT MSVC)
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build)
endif()

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

# Global vars
configure_file(
	${CMAKE_SOURCE_DIR}/src/Runtime.hpp.in
	${PROJECT_BINARY_DIR}/Runtime.hpp @ONLY
)
	
# Libraries 
find_package(tsl-ordered-map CONFIG REQUIRED)
include_directories(/usr/include/readline)

# Main project CMake
add_subdirectory(src)
target_include_directories(Runtime PUBLIC ${PROJECT_BINARY_DIR}) # Runtime.h
target_link_libraries( Runtime PUBLIC tsl::ordered_map readline ffi)

# Tests and coverage
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
if (RUNTIME_BUILD_TESTS)
	# Coverage
	if(CMAKE_COMPILER_IS_GNUCXX)
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --coverage -g -O0")
		set(CMAKE_CXX_FLAGS " ${CMAKE_CXX_FLAGS} --coverage -g -O0")
		set(GCOVR_ADDITIONAL_ARGS
			"--exclude-unreachable-branches"
			"--exclude-throw-branches")
		include(CodeCoverage)
		setup_target_for_coverage_gcovr_html(
			NAME ${PROJECT_NAME}_coverage
			EXECUTABLE ${CMAKE_CURRENT_BINARY_DIR}/tests_runtime
			EXCLUDE "${PROJECT_BINARY_DIR}/vcpkg_installed" # Don't include vcpkg libraries
		)
		# You still need to manually run `ninja Runtime_coverage` for whatever reason
	endif()

	# Tests
	add_subdirectory(tests)
	enable_testing()
	add_test(
		NAME Runtime_test
		COMMAND ${CMAKE_CURRENT_BINARY_DIR}/tests_runtime
	)
	target_include_directories(tests_runtime PRIVATE ${PROJECT_BINARY_DIR}) # Runtime.h
endif()

# Packaging
if (RUNTIME_INSTALL)

	# Generic
	set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Small interpreted programming language")
	set(CPACK_PACKAGE_CONTACT "Wisdurm")
	set(CPACK_GENERATOR "DEB;TZ;TGZ")
	# Debian
	set(CPACK_DEBIAN_PACKAGE_DEPENDS "libreadline8t64 libffi8")
	# Windows

	if(RUNTIME_BUILD_TESTS) # For debugging this might be useful
		install(TARGETS Runtime tests_runtime RUNTIME DESTINATION bin)
	else()
		install(TARGETS Runtime RUNTIME DESTINATION bin)
	endif()
	include(CPack)
endif()
