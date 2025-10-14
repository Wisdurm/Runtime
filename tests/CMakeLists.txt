# Tests CMakeLists.txt

enable_testing()

# Tests using Catch2
add_subdirectory( ${CMAKE_SOURCE_DIR}/external/Catch2 build )
add_executable( tests_runtime
${CMAKE_SOURCE_DIR}/tests/tokenizer_tests.cpp
${CMAKE_SOURCE_DIR}/tests/interpreter_tests.cpp
${CMAKE_SOURCE_DIR}/tests/parser_tests.cpp
${CMAKE_SOURCE_DIR}/src/compiler/tokenizer.cpp
${CMAKE_SOURCE_DIR}/src/compiler/parser.cpp
${CMAKE_SOURCE_DIR}/src/compiler/interpreter.cpp
${CMAKE_SOURCE_DIR}/src/flowgraph.cpp
)
target_link_libraries( tests_runtime PRIVATE Catch2::Catch2WithMain tsl::ordered_map)
# Enforce C++ 20 (again)
set_property(TARGET tests_runtime PROPERTY CXX_STANDARD 20)