// Catch 2
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_exception.hpp>
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
	auto r1 = rt::parse(rt::tokenize("Object(str, Print(\"NULL\"), \"Hello world\")\nPrint(str)"));
	REQUIRE(rt::interpretAndReturn(r1.get())[0] == test1); // Print "Hello world", but not "NULL"
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
	auto r1 = rt::parse(rt::tokenize("Object(num, 1)\nAssign(num, 0, 2)\nPrint(num)"));
	REQUIRE(rt::interpretAndReturn(r1.get())[0] == test1);
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
	auto r1 = rt::parse(rt::tokenize("Object(args, \"Hi\", \"Hello\")\nargs-0\nPrint(args-0)"));
	REQUIRE(rt::interpretAndReturn(r1.get())[0] == test1);

	// Object(Main,
	//	Assign(args-0, 0, "Hi"),
	//	Print(args-0-0),
	// )
	// Excepted output: "Hi"

	const std::string test2 = "Hi";
	auto r2 = rt::parse(rt::tokenize("Assign(args-0, 0, \"Hi\")\nPrint(args-0-0)"));
	REQUIRE(rt::interpretAndReturn(r2.get())[0] == test2);

	// Object(Main,
	//	Assign(named, "Hello", "Hi"),
	//	Print(named-"Hello"),
	// )
	// Expected output: "Hi"

	const std::string test3 = "Hi";
	auto r3 = rt::parse(rt::tokenize("Assign(named, \"Hello\", \"Hi\")\nPrint(named-\"Hello\")"));
	REQUIRE(rt::interpretAndReturn(r3.get())[0] == test3);

	// Object(Main,
	//	Object(zero, 0),
	//	Object(values, "Hi", "Hello"),
	//	Print(values-zero()),
	// )
	// Expected output: "Hi"

	const std::string test4 = "Hi";
	auto r4 = rt::parse(rt::tokenize("Object(zero,0)\nObject(values,\"Hi\",\"Hello\")\nPrint(values-zero())"));
	REQUIRE(rt::interpretAndReturn(r4.get())[0] == test4);
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
  
	std::string test1[] { "1.000000", "2.000000" };
	auto r1 = rt::parse(rt::tokenize("Object(one, 1)\nObject(value, one-0)\nObject(reference, one)\nAssign(one, 0, 2)\nPrint(value)\nPrint(reference)"));
	auto v = rt::interpretAndReturn(r1);
	REQUIRE(v[0] == test1[0]);
	REQUIRE(v[1] == test1[1]);
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
  
	std::string test1 []{ "0.000000", "1.000000" };
	auto r1 = rt::parse(rt::tokenize("Object(PrintArg,Print(arg))\nPrintArg()\nPrintArg(1)"));
	auto v = rt::interpretAndReturn(r1);
	REQUIRE(v[0] == test1[0]);
	REQUIRE(v[1] == test1[1]);

	// Object(Main,
	//	Object(PrintArg,
	//		Print(argone),
	//		Print(argtwo),
	//	),
	//	PrintArg(1),
	//	PrintArg(1,2),
	//	)
	// Expected output: "1\n0\n1\n2"
  
	std::string test2[]{ "1.000000", "0.000000", "1.000000", "2.000000"};
	auto r2 = rt::parse(rt::tokenize("Object(PrintArg,Print(argone),Print(argtwo))\nPrintArg(1)\nPrintArg(1,2)"));
  auto v2 = rt::interpretAndReturn(r2);
	REQUIRE(v2[0] == test2[0]);
	REQUIRE(v2[1] == test2[1]);
	REQUIRE(v2[2] == test2[2]);
	REQUIRE(v2[3] == test2[3]);

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

	std::string test3[]{ "0.000000", "1.000000" };
	ast::Expression* r3 = rt::parse(rt::tokenize("Object(Display,Print(arg))\nObject(PrintArg,Display(arg))\nPrintArg()\nPrintArg(1)"));
	// TODO: yeah
	auto v3 = rt::interpretAndReturn(r3);
	REQUIRE(v3[0] == test3[0]);
	REQUIRE(v3[1] == test3[1]);
	delete r3;
}

TEST_CASE("Member functions", "[interpreter]")
{
	// Object(Main,
	//	Object(prt, Print("Hi"))
	//	Object(obj, Print("Hello"), prt)
	//	obj-1()
	//	obj-0()
	// )
	// Expected output: "Hi\nHello"
	std::string test1[]{ "Hi", "Hello" };
	auto r1 = rt::parse(rt::tokenize("Object(prt, Print(\"Hi\"))"
				"Object(obj, Print(\"Hello\"), prt)"
				"obj-1()"
				"obj-0()"));
	auto v = rt::interpretAndReturn(r1.get());
	REQUIRE(v[0] == test1[0]);
	REQUIRE(v[1] == test1[1]);
}

TEST_CASE("Exceptions", "[interpreter]")
{
	// Object(Main,
	//	Object(r, Print(r))
	//	r()
	// )
	// Expected output: Exception
	ast::Expression* r1 = rt::parse(rt::tokenize("Object(r,Print(r)) r()"));
	REQUIRE_THROWS_WITH(rt::interpretAndReturn(r1), "Object evaluation got stuck in an infinite loop");
	delete r1;

	// Object(Main,
	// 	Print(Print)
	// )
	// Expected output: Exception
	ast::Expression* r2 = rt::parse(rt::tokenize("Print(Print)"));
	REQUIRE_THROWS_WITH(rt::interpretAndReturn(r2), "Attempt to evaluate built-in function");
	delete r2;
}

TEST_CASE("Runtime exceptions", "[interpreter]")
{
	// Object(Main,
	//	Print(Object(1))
	// )
	// Expected output: Exception
	std::string test1 = "Incorrect arguments";
	ast::Expression* r1 = rt::parse(rt::tokenize("Print(Object(1))"));
	auto v = rt::interpretAndReturn(r1);
	REQUIRE(v[0] == test1);
	delete r1;
}
