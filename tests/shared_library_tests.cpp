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

	auto r1 = rt::parse(rt::tokenize("Include(\"../tests/lib.so\")"
				"test(5)"));
	REQUIRE_THROWS_WITH(rt::interpretAndReturn(r1), "Shared function is not yet bound");

	// Object(Main,
	//	Bind("bruh","void")
	// )
	// Excepted output: Exception

	auto r2 = rt::parse(rt::tokenize("Bind(\"bruh\",\"void\")"));
	REQUIRE_THROWS_WITH(rt::interpretAndReturn(r2), "Unable to find symbol");
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
	auto r1 = rt::parse(rt::tokenize("Include(\"../tests/lib.so\")"
				"Bind(\"test\",\"int\",\"int\")"
				"Print(test(5))"));
	REQUIRE(rt::interpretAndReturn(r1)[0] == test1);

	// Object(Main,
	//	Include("../tests/lib.so")
	//	Bind("testVoid", "void")
	//	Print(testVoid())
	// )
	// Excepted output: Program doesn't crash :)

	const std::string test2 = "10.000000";
	auto r2 = rt::parse(rt::tokenize("Include(\"../tests/lib.so\")"
				"Bind(\"testVoid\",\"void\")"
				"Print(testVoid())"));
	REQUIRE_NOTHROW(rt::interpretAndReturn(r2));
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
	auto r1 = rt::parse(rt::tokenize("Include(\"../tests/lib.so\")"
				"Bind(\"compareStr\",\"void\",\"cstring\", \"cstring\")"
				"Print(compareStr(\"Hello\", \"Hello\"))"));
	REQUIRE(rt::interpretAndReturn(r1)[0] == test1);
}

TEST_CASE("Struct argument", "[shared_libraries]")
{
	// Object(Main,
	//	Include("../tests/lib.so")
	//	Object(structure, "int", "float")
	//	Bind("compareStruct", "int", structure)
	//	Object(sr, 2.9, 2.1) # 2.9 gets truncated to 2
	//	Print(compareStruct(sr))
	// )
	// Excepted output: "0" (False) (2 is not larger than 2.1)

	const std::string test1 = "0.000000";
	auto r1 = rt::parse(rt::tokenize("Include(\"../tests/lib.so\")"
				"Object(structure, \"int\", \"float\")"
				"Bind(\"compareStruct\",\"int\",structure)"
				"Object(sr, 2.9, 2.1)"
				"Print(compareStruct(sr))"));
	REQUIRE(rt::interpretAndReturn(r1)[0] == test1);
}

TEST_CASE("Pointer argument", "[shared_libraries]")
{
	// Object(Main,
	//	Include("../tests/lib.so")
	//	Bind("triplePtr", "void", "int*")
	//	Object(i, 7)
	//	Print(i)
	//	triplePtr(i)
	//	Print(i)
	// )
	// Excepted output: "7\n21"

	const std::string test1[]{"7.000000", "21.000000"};
	auto r1 = rt::parse(rt::tokenize("Include(\"../tests/lib.so\")"
				"Bind(\"triplePtr\", \"void\", \"int*\")"
				"Object(i, 7)"
				"Print(i)"
				"triplePtr(i)"
				"Print(i)"));
	auto v1 = rt::interpretAndReturn(r1); 
	REQUIRE(v1[0] == test1[0]);
	REQUIRE(v1[1] == test1[1]);
}

TEST_CASE("Struct return value", "[shared_libraries]")
{
	// Object(Main,
	//	Include("../tests/lib.so")
	//	Object(structure, "int", "float")
	//	Bind("retStruct", structure, "int", "float")
	//	Print(retStruct(2, 5))
	// )
	// Excepted output: "5"

	const std::string test1 = "5.000000";
	auto r1 = rt::parse(rt::tokenize("Include(\"../tests/lib.so\")"
				"Object(structure, \"int\", \"float\")"
				"Bind(\"retStruct\", structure, \"int\", \"float\")"
				"Print(retStruct(2, 5))"));
	auto v1 = rt::interpretAndReturn(r1); 
	REQUIRE(v1[0] == test1);
}
