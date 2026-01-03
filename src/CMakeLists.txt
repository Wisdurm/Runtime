# Main project CMakeLists.txt

add_executable ( Runtime
# Main
${CMAKE_SOURCE_DIR}/src/Runtime.cpp
# Tokenizer
${CMAKE_SOURCE_DIR}/src/compiler/tokenizer.cpp
# Parser
${CMAKE_SOURCE_DIR}/src/compiler/parser.cpp
# Interpreter
${CMAKE_SOURCE_DIR}/src/compiler/interpreter.cpp
)

# Enforce C++ 20 (again)
set_property(TARGET Runtime PROPERTY CXX_STANDARD 20)
