// Catch 2
#include <catch2/catch_test_macros.hpp>
// Runtime
#include "../src/compiler/tokenizer.h"
#include "../src/compiler/parser.h"
#include "../src/compiler/interpreter.h"
// C++
#include <vector>

TEST_CASE("Object creation and console output", "[interpreter]")
{
	// Test object creation and basic Hello world output
	const std::vector<std::string> test1 = { "Hello world" };
	ast::Expression* r1 = rt::parse(rt::tokenize("Object(str, Print(\"NULL\"), Object(str), \"Hello world\")\nPrint(str)"));
	REQUIRE(rt::interpretAndReturn(r1)->at(0) == test1.data()->at(0)); // Print "Hello world", but not "NULL"
	delete r1;
}