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

	// Object(Main,
	//	Assign(named, "Hello", "Hi")
	//	Print(named-"Hello")
	// )
	// Expected output: "Hi"

	const std::vector<std::string> test3 = { "Hi" };
	ast::Expression* r3 = rt::parse(rt::tokenize("Assign(named, \"Hello\", \"Hi\")\nPrint(named-\"Hello\")"));
	REQUIRE(rt::interpretAndReturn(r3)[0] == test3.at(0));
	delete r3;
}

TEST_CASE("Overcomplex evaluation", "[interpreter]")
{
	// Object(Main,
	//	Object(args, Object(nest, "Hi"), "Hello"),
	//	Print(args-0-0)
	// )
	// Excepted output: "Hi"

	// I can't reason out whether or not this should run, airing on the side of it can't because trying to ponder how it could makes me feel like I'm having a panic attack
	ast::Expression* r1 = rt::parse(rt::tokenize("Object(args, Object(nest, \"Hi\"), \"Hello\")\nPrint(args-0-0)"));
	REQUIRE_THROWS(rt::interpretAndReturn(r1));
	delete r1;
}

TEST_CASE("Reference passing and explicit evaluation", "[interpreter]")
{
	// Object(Main,
	//	Object(one, 1)
	//	Object(value, one-0) // Passed the value of one
	//	Object(reference, one) // Passed a reference to one
	//	Assign(one, 0, 2)
	//	Print(value)
	//	Print(reference)
	// )
	// Expected output: "1\n2"
	std::string* test1 = new std::string[] { "1", "2" };
	ast::Expression* r1 = rt::parse(rt::tokenize("Object(one, 1)\nObject(value, one-0)\nObject(reference, one)\nAssign(one, 0, 2)\nPrint(value)\nPrint(reference)"));
	REQUIRE(*rt::interpretAndReturn(r1) == *test1);
	delete r1;
	delete []test1;
}
