# Add source to this project's executable.
add_executable ( Runtime
# Main
${CMAKE_SOURCE_DIR}/src/Runtime.cpp
${CMAKE_SOURCE_DIR}/src/Runtime.h
# Tokenizer
${CMAKE_SOURCE_DIR}/src/compiler/tokenizer.cpp
${CMAKE_SOURCE_DIR}/src/compiler/tokenizer.h
# Parser
${CMAKE_SOURCE_DIR}/src/compiler/parser.cpp
${CMAKE_SOURCE_DIR}/src/compiler/parser.h
${CMAKE_SOURCE_DIR}/src/compiler/ast.h
# Interpreter
${CMAKE_SOURCE_DIR}/src/compiler/interpreter.h
${CMAKE_SOURCE_DIR}/src/compiler/interpreter.cpp
# Runtime standard libraries
${CMAKE_SOURCE_DIR}/src/compiler/Lib/StandardLibrary.h
)