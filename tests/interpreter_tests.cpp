// Catch 2
#include <catch2/catch_test_macros.hpp>
// Runtime
#include "../src/compiler/tokenizer.h"
#include "../src/compiler/parser.h"
#include "../src/compiler/interpreter.h"
// C++
#include <vector>

TEST_CASE("Object creation, basic evaluation and console output", "[interpreter]")
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
	// Excepted output: "2"

	const std::vector<std::string> test1 = { "2" };
	ast::Expression* r1 = rt::parse(rt::tokenize("Object(num, 1)\nAssign(num, 0, 2)\nPrint(num)"));
	REQUIRE(rt::interpretAndReturn(r1)[0] == test1.at(0));
	delete r1;
}

TEST_CASE("Member accession", "[interpreter]")
{
	// Object(Main,
	//	Object(args, "Hi", "Hello"),
	//	args-0 # This is here to test interpreter implementation
	//	Print(args-0)
	// )
	// Excepted output: "Hi"

	const std::vector<std::string> test1 = { "Hi" };
	ast::Expression* r1 = rt::parse(rt::tokenize("Object(args, \"Hi\", \"Hello\")\nargs-0\nPrint(args-0)"));
	REQUIRE(rt::interpretAndReturn(r1)[0] == test1.at(0));
	delete r1;

	// Object(Main,
	//	Assign(args-0, 0, "Hi"),
	//	Print(args-0-0)
	// )
	// Excepted output: "Hi"

	const std::vector<std::string> test2 = { "Hi" };
	ast::Expression* r2 = rt::parse(rt::tokenize("Assign(args-0, 0, \"Hi\")\nPrint(args-0-0)"));
	REQUIRE(rt::interpretAndReturn(r2)[0] == test2.at(0));
	delete r2;
}

TEST_CASE("Complex evaluation", "[interpreter]")
{
	// Object(Main,
	//	Object(args, Object("Hi", "Hello"), "Hello"),
	//	Print(args-0-0)
	// )
	// Excepted output: "Hi"

	// nightmare nightmare nightmare nightmare nightmare nightmare nightmare nightmare 
}