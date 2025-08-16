// Catch 2
#include <catch2/catch_test_macros.hpp>
// Runtime
#include "../src/compiler/tokenizer.h"
#include "../src/compiler/parser.h"
#include "../src/compiler/interpreter.h"
// C++
#include <vector>

TEST_CASE("Object creation, complex evaluation and console output", "[interpreter]")
{
	// Object(Main,
	//	Object(str, Print("NULL"), Object(obj, "Hello world")),
	//	Print(str)
	// )
	// Excepted output: "Hello world"

	const std::vector<std::string> test1 = { "Hello world" };
	ast::Expression* r1 = rt::parse(rt::tokenize("Object(str, Print(\"NULL\"), Object(obj, \"Hello world\"))\nPrint(str)"));
	REQUIRE(rt::interpretAndReturn(r1)[0] == test1.at(0)); // Print "Hello world", but not "NULL"
	delete r1;
}

TEST_CASE("Assignment", "[interpreter]")
{
	// Object(Main,
	//	Object(num, 1),
	//  Assign(num, 1, 2)
	//	Print(num)
	// )

	const std::vector<std::string> test1 = { "2" };
	ast::Expression* r1 = rt::parse(rt::tokenize("Object(num, 1)\nAssign(num, 0, 2)\nPrint(num)"));
	REQUIRE(rt::interpretAndReturn(r1)[0] == test1.at(0)); // Print 2
	delete r1;
}