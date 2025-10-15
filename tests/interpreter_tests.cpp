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
	//	Object(str, Print("NULL"), "Hello world"),
	//	Print(str),
	// )
	// Excepted output: "Hello world"

	const std::string test1 = "Hello world";
	ast::Expression* r1 = rt::parse(rt::tokenize("Object(str, Print(\"NULL\"), \"Hello world\")\nPrint(str)"));
	REQUIRE(rt::interpretAndReturn(r1)[0] == test1); // Print "Hello world", but not "NULL"
	delete r1;
}

TEST_CASE("Assignment", "[interpreter]")
{
	// Object(Main,
	//	Object(num, 1),
	//  Assign(num, 1, 2),
	//	Print(num),
	// )
	// Excepted output: "2"

	const std::string test1 = "2.000000";
	ast::Expression* r1 = rt::parse(rt::tokenize("Object(num, 1)\nAssign(num, 0, 2)\nPrint(num)"));
	REQUIRE(rt::interpretAndReturn(r1)[0] == test1);
	delete r1;
}

TEST_CASE("Member accession", "[interpreter]")
{
	// Object(Main,
	//	Object(args, "Hi", "Hello"),
	//	args-0, # This is here to test interpreter implementation
	//	Print(args-0),
	// )
	// Excepted output: "Hi"

	const std::string test1 = "Hi";
	ast::Expression* r1 = rt::parse(rt::tokenize("Object(args, \"Hi\", \"Hello\")\nargs-0\nPrint(args-0)"));
	REQUIRE(rt::interpretAndReturn(r1)[0] == test1);
	delete r1;

	// Object(Main,
	//	Assign(args-0, 0, "Hi"),
	//	Print(args-0-0),
	// )
	// Excepted output: "Hi"

	const std::string test2 = "Hi";
	ast::Expression* r2 = rt::parse(rt::tokenize("Assign(args-0, 0, \"Hi\")\nPrint(args-0-0)"));
	REQUIRE(rt::interpretAndReturn(r2)[0] == test2);
	delete r2;

	// Object(Main,
	//	Assign(named, "Hello", "Hi"),
	//	Print(named-"Hello"),
	// )
	// Expected output: "Hi"

	const std::string test3 = "Hi";
	ast::Expression* r3 = rt::parse(rt::tokenize("Assign(named, \"Hello\", \"Hi\")\nPrint(named-\"Hello\")"));
	REQUIRE(rt::interpretAndReturn(r3)[0] == test3);
	delete r3;

	// Object(Main,
	//	Object(zero, 0),
	//	Object(values, "Hi", "Hello"),
	//	Print(values-zero()),
	// )
	// Expected output: "Hi"

	const std::string test4 = "Hi";
	ast::Expression* r4 = rt::parse(rt::tokenize("Object(zero,0)\nObject(values,\"Hi\",\"Hello\")\nPrint(values-zero())"));
	REQUIRE(rt::interpretAndReturn(r4)[0] == test4);
	delete r4;
}

TEST_CASE("Reference passing and explicit evaluation", "[interpreter]")
{
	// Object(Main,
	//	Object(one, 1),
	//	Object(value, one-0), // Passed the value of one
	//	Object(reference, one), // Passed a reference to one
	//	Assign(one, 0, 2),
	//	Print(value),
	//	Print(reference),
	// )
	// Expected output: "1\n2"
	std::string* test1 = new std::string[] { "1.000000", "2.000000" };
	ast::Expression* r1 = rt::parse(rt::tokenize("Object(one, 1)\nObject(value, one-0)\nObject(reference, one)\nAssign(one, 0, 2)\nPrint(value)\nPrint(reference)"));
	auto v = rt::interpretAndReturn(r1);
	REQUIRE(v[0] == test1[0]);
	REQUIRE(v[1] == test1[1]);
	delete r1;
	delete []test1;
}

TEST_CASE("Params", "[interpreter]")
{
	// Object(Main,
	//	Object(PrintArg,
	//		Print(arg),
	//	),
	//	PrintArg(),
	//	PrintArg(1),
	//	)
	// Expected output: "0\n1"
	std::string* test1 = new std::string[]{ "0.000000", "1.000000" };
	ast::Expression* r1 = rt::parse(rt::tokenize("Object(PrintArg,Print(arg))\nPrintArg()\nPrintArg(1)"));
	auto v = rt::interpretAndReturn(r1);
	REQUIRE(v[0] == test1[0]);
	REQUIRE(v[1] == test1[1]);
	delete r1;
	delete[]test1;

	// Object(Main,
	//	Object(PrintArg,
	//		Print(argone),
	//		Print(argtwo),
	//	),
	//	PrintArg(1),
	//	PrintArg(1,2),
	//	)
	// Expected output: "1\n0\n1\n2"
	std::string* test2 = new std::string[]{ "1.000000", "0.000000", "1.000000", "2.000000"};
	ast::Expression* r2 = rt::parse(rt::tokenize("Object(PrintArg,Print(argone),Print(argtwo))\nPrintArg(1)\nPrintArg(1,2)"));
	auto v2 = rt::interpretAndReturn(r2);
	REQUIRE(v2[0] == test2[0]);
	REQUIRE(v2[1] == test2[1]);
	REQUIRE(v2[2] == test2[2]);
	REQUIRE(v2[3] == test2[3]);
	delete r2;
	delete[]test2;

	// Hierachical

	// Object(Main,
	//	Object(Display,
	//		Print(arg)
	//	),
	//	Object(PrintArg,
	//		Display(arg),
	//	),
	//	PrintArg(),
	//	PrintArg(1),
	//	)
	// Expected output: "0\n1"
	/*std::string* test2 = new std::string[]{ "0", "1" };
	ast::Expression* r2 = rt::parse(rt::tokenize("Object(PrintArg,Print(arg))\nPrintArg()\nPrintArg(1)"));
	auto v2 = rt::interpretAndReturn(r2);
	REQUIRE(v2[0] == test2[0]);
	REQUIRE(v2[1] == test2[1]);
	delete r2;
	delete[]test2;*/
}