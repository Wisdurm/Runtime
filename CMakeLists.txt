# CMakeList.txt : CMake project for Runtime, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)
project (Runtime)

# Output
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

add_subdirectory(src)
add_subdirectory(tests)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET Runtime PROPERTY CXX_STANDARD 20)
  set_property(TARGET tests PROPERTY CXX_STANDARD 20)
endif()