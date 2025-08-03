# Tests using Catch2
add_subdirectory( ${CMAKE_SOURCE_DIR}/external/Catch2 build )
add_executable( tests 
${CMAKE_SOURCE_DIR}/tests/tokenizer_tests.cpp
${CMAKE_SOURCE_DIR}/tests/interpreter_tests.cpp
${CMAKE_SOURCE_DIR}/tests/parser_tests.cpp
${CMAKE_SOURCE_DIR}/src/compiler/tokenizer.cpp
${CMAKE_SOURCE_DIR}/src/compiler/parser.cpp
${CMAKE_SOURCE_DIR}/src/compiler/interpreter.cpp
)
target_link_libraries( tests PRIVATE Catch2::Catch2WithMain )