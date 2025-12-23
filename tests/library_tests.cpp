// Catch 2
#include <catch2/catch_test_macros.hpp>
// Runtime
#include "../src/compiler/tokenizer.h"
#include "../src/compiler/parser.h"
#include "../src/compiler/interpreter.h"
// C++
#include <vector>

TEST_CASE("Basic conditionals and loops", "[interpreter]")
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
	ast::Expression* r1 = rt::parse(rt::tokenize("If(Not(False) Print(\"True\") Print(\"False\"))"));
	REQUIRE(rt::interpretAndReturn(r1)[0] == test1);
	delete r1;

	// Object(Main,
	//	While(SmallerThan(i, 10)
	//		Assign(i,0,Add(i,1))
	//	)
	//	Print(i)
	// )
	// Excepted output: "9"

	const std::string test2 = "10.000000";
	ast::Expression* r2 = rt::parse(rt::tokenize("While(SmallerThan(i,10)"
				"Assign(i,0,Add(i,1))"
				")"
				"Print(i)"));
	REQUIRE(rt::interpretAndReturn(r2)[0] == test2);
	delete r2;
}


