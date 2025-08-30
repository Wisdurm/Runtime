# Main project CMakeLists.txt

add_executable ( Runtime
# Main
${CMAKE_SOURCE_DIR}/src/Runtime.cpp
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
${CMAKE_SOURCE_DIR}/src/compiler/Stlib/StandardLibrary.h
# Flowgraph
${CMAKE_SOURCE_DIR}/src/flowgraph.cpp
${CMAKE_SOURCE_DIR}/src/flowgraph.h
)

# Enforce C++ 20 (again)
set_property(TARGET Runtime PROPERTY CXX_STANDARD 20)