// Catch 2
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_exception.hpp>
// Runtime
#include "../src/compiler/tokenizer.h"
#include "../src/compiler/parser.h"
#include "../src/compiler/interpreter.h"

TEST_CASE("Exceptions", "[shared_libraries]")
{
	// Object(Main,
	//	Include("../tests/lib.so")
	//	test(5)
	// )
	// Excepted output: Exception

	ast::Expression* r1 = rt::parse(rt::tokenize("Include(\"../tests/lib.so\")"
				"test(5)"));
	REQUIRE_THROWS_WITH(rt::interpretAndReturn(r1), "Shared function is not yet bound");
	delete r1;

	// Object(Main,
	//	Bind("bruh","void")
	// )
	// Excepted output: Exception

	ast::Expression* r2 = rt::parse(rt::tokenize("Bind(\"bruh\",\"void\")"));
	REQUIRE_THROWS_WITH(rt::interpretAndReturn(r2), "Unable to find symbol");
	delete r2;
}

TEST_CASE("Basic function calling", "[shared_libraries]")
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

TEST_CASE("Multiple string arguments", "[shared_libraries]")
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

TEST_CASE("Struct argument", "[shared_libraries]")
{
	// Object(Main,
	//	Include("../tests/lib.so")
	//	Object(structure, "int", "float")
	//	Bind("compareStruct", "int", structure)
	//	Object(sr, 2.4, 2.2) # 2.4 gets truncated to 2
	//	Print(compareStruct(sr))
	// )
	// Excepted output: "0" (False)

	const std::string test1 = "0.000000";
	ast::Expression* r1 = rt::parse(rt::tokenize("Include(\"../tests/lib.so\")"
				"Object(structure, \"int\", \"float\")"
				"Bind(\"compareStruct\",\"int\",structure)"
				"Object(sr, 2.4, 2.2)"
				"Print(compareStruct(sr))"));
	REQUIRE(rt::interpretAndReturn(r1)[0] == test1);
	delete r1;
}
