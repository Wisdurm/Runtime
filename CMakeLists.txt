# Root CMakeLists.txt

# Specify C++ 20
add_compile_options(-std=c++20)

cmake_minimum_required (VERSION 3.20)
project (Runtime
	LANGUAGES CXX
	VERSION 0.9.7
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
	${CMAKE_SOURCE_DIR}/src/Runtime.h.in
	${PROJECT_BINARY_DIR}/Runtime.h @ONLY
)
	
# Libraries 
find_package(tsl-ordered-map CONFIG REQUIRED)

# Main project CMake
add_subdirectory(src)
target_include_directories(Runtime PUBLIC ${PROJECT_BINARY_DIR}) # Runtime.h
target_link_libraries( Runtime PUBLIC tsl::ordered_map)

# Tests
if (RUNTIME_BUILD_TESTS)
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
	# Debian
	set(CPACK_GENERATOR "DEB")
	# Windows

	if(RUNTIME_BUILD_TESTS) # For debugging this might be useful
		install(TARGETS Runtime tests_runtime RUNTIME DESTINATION bin)
	else()
		install(TARGETS Runtime RUNTIME DESTINATION bin)
	endif()
	include(CPack)
endif()
