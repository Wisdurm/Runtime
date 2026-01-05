// Catch 2
#include <catch2/catch_test_macros.hpp>
// Runtime
#include "../src/compiler/tokenizer.h"
#include "../src/compiler/parser.h"
#include "../src/compiler/interpreter.h"
// C++
#include <vector>

TEST_CASE("Basic conditionals and loops", "[libraries]")
{
	// Object(Main,
	//	If(Not(False)
	//		# Then
	//		Print("True")
	//		# Else
	//		Print("False")
	//	)
	// )
	// Excepted output: "True"

	const std::string test1 = "True";
	auto r1 = rt::parse(rt::tokenize("If(Not(False) Print(\"True\") Print(\"False\"))"));
	REQUIRE(rt::interpretAndReturn(r1)[0] == test1);

	// Object(Main,
	//	While(SmallerThan(i, 10)
	//		Assign(i,0,Add(i,1))
	//	)
	//	Print(i)
	// )
	// Excepted output: "9"

	const std::string test2 = "10.000000";
	auto r2 = rt::parse(rt::tokenize("While(SmallerThan(i,10)"
				"Assign(i,0,Add(i,1))"
				")"
				"Print(i)"));
	REQUIRE(rt::interpretAndReturn(r2)[0] == test2);
}

TEST_CASE("Runtime libraries", "[libraries]")
{
	// Object(Main,
	//	Include("../tests/test.rnt")
	//	Print(testObj-1)
	//	Print(Size(testObj))
	// )
	// Excepted output: "2\n3"

	const std::string test1[]{"2.000000", "4.000000"};
	auto r1 = rt::parse(rt::tokenize("Include(\"../tests/test.rnt\")"
				"Print(testObj-1)"
				"Print(Size(testObj))"));
	auto v1 = rt::interpretAndReturn(r1);
	REQUIRE(v1[0] == test1[0]);
	REQUIRE(v1[1] == test1[1]);
}

TEST_CASE("Format", "[libraries]")
{
	// Object(Main,
	// 	Print(Format("0 == $2\n", False))
	// )
	// Excepted output: "0 == 0.00\n"

	const std::string test1 = "0 == 0.00\n";
	auto r1 = rt::parse(rt::tokenize("Print(Format(\"0 == $2\\n\", False))"));
	REQUIRE(rt::interpretAndReturn(r1)[0] == test1);
}
