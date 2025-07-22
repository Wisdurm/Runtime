# CMakeList.txt : CMake project for Runtime, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("Runtime")

# Add source to this project's executable.
add_executable (Runtime "src/Runtime.cpp" "src/Runtime.h" "src/compiler/tokenizer.cpp" "src/compiler/tokenizer.h"  "tests/tokenizer_tests.h" "src/compiler/parser.cpp" "src/compiler/parser.h" "src/compiler/ast.h" "tests/parser_tests.h" "src/compiler/interpreter.h" "src/compiler/interpreter.cpp" "src/compiler/Lib/StandardLibrary.h" "tests/interpreter_tests.h")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET Runtime PROPERTY CXX_STANDARD 20)
endif()

# TODO: Add tests and install targets if needed.
