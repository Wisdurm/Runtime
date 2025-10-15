# Root CMakeLists.txt

# Specify C++ 20
add_compile_options(-std=c++20)

cmake_minimum_required (VERSION 3.20)
project (Runtime
	LANGUAGES CXX
	VERSION 0.9.5
)

# Options
option(RUNTIME_BUILD_TESTS "Configure and build tests" TRUE)

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
add_subdirectory( ${CMAKE_SOURCE_DIR}/external/Tessil )

# Main project CMake
add_subdirectory(src)
target_include_directories(Runtime PUBLIC ${PROJECT_BINARY_DIR})
target_link_libraries( Runtime PRIVATE tsl::ordered_map)

# Tests
if (RUNTIME_BUILD_TESTS)
	add_subdirectory(tests)
	enable_testing()
	add_test(
		NAME Runtime_test
		COMMAND ${CMAKE_CURRENT_BINARY_DIR}/tests_runtime
	)
	target_include_directories(tests_runtime PUBLIC ${PROJECT_BINARY_DIR}) # Runtime.h
endif()
