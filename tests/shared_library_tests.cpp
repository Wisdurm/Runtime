// Catch 2
#include <catch2/catch_test_macros.hpp>
// Runtime
#include "../src/compiler/tokenizer.h"
#include "../src/compiler/parser.h"
#include "../src/compiler/interpreter.h"

TEST_CASE("Basic function calling", "[interpreter]")
{
	// Object(Main,
	//	Include("../tests/lib.so")
	//	Bind("test", "int", "int")
	//	Print(test(5))
	// )
	// Excepted output: "10"

	const std::string test1 = "10.000000";
	ast::Expression* r1 = rt::parse(rt::tokenize("Include(\"../tests/lib.so\")"
				"Bind(\"test\",\"int\",\"int\")"
				"Print(test(5))"));
	REQUIRE(rt::interpretAndReturn(r1)[0] == test1);
	delete r1;

	// Object(Main,
	//	Include("../tests/lib.so")
	//	Bind("testVoid", "void")
	//	Print(testVoid())
	// )
	// Excepted output: Program doesn't crash :)

	const std::string test2 = "10.000000";
	ast::Expression* r2 = rt::parse(rt::tokenize("Include(\"../tests/lib.so\")"
				"Bind(\"testVoid\",\"void\")"
				"Print(testVoid())"));
	REQUIRE_NOTHROW(rt::interpretAndReturn(r2));
	delete r2;
}

TEST_CASE("Multiple string arguments", "[interpreter]")
{
	// Object(Main,
	//	Include("../tests/lib.so")
	//	Bind("compareStr", "void", "cstring", "cstring")
	//	Print(compareStr("Hello", "Hello"))
	// )
	// Excepted output: "1" (True)

	const std::string test1 = "1.000000";
	ast::Expression* r1 = rt::parse(rt::tokenize("Include(\"../tests/lib.so\")"
				"Bind(\"compareStr\",\"void\",\"cstring\", \"cstring\")"
				"Print(compareStr(\"Hello\", \"Hello\"))"));
	REQUIRE(rt::interpretAndReturn(r1)[0] == test1);
	delete r1;
}
