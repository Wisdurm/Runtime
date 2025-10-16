// Catch 2
#include <catch2/catch_test_macros.hpp>
// Runtime
#include "../src/compiler/tokenizer.h"
#include "../src/compiler/parser.h"
// C++
#include <variant>

//TODO: REMOVE
#include "../src/flowgraph.h"
#include <iostream>

/// <summary>
/// Placeholder values for code locations
/// </summary>
static const rt::SourceLocation L;

using namespace rt;

TEST_CASE("Expression comparison", "[parser]")
{
	// Verify operator== works
	ast::Expression* r1 = new ast::Call(L, new ast::Identifier(L, "Print"), {new ast::Literal(L, 2.0), new ast::Identifier(L, "asd")});
	ast::Expression* r2 = new ast::Call(L, new ast::Identifier(L, "Print"), { new ast::Literal(L, 2.0), new ast::Identifier(L, "asd")});
	REQUIRE(*r1 == *r2);
	delete r1;
	delete r2;
}

TEST_CASE("Literals", "[parser]")
{
	const char* test1 = "Object(Main, 1, 1.2, \"1.2\")";
	ast::Expression* r1 = new ast::Call(L, new ast::Identifier(L, "Object"), { new ast::Identifier(L,"Main"), new ast::Literal(L, 1.0), new ast::Literal(L, 1.2) , new ast::Literal(L, std::variant<double, std::string>("1.2")) });
	auto v = parse(tokenize(test1));
	REQUIRE(*v == *r1);
	delete r1;
}

TEST_CASE("Top level statements", "[parser]")
{
	// Test basic top level statements
	const char* test1 = "i\nAssign(i-0,1)";
	ast::Expression* r1 = new ast::Call(L, new ast::Identifier(L, "Object"), { new ast::Identifier(L,"Main"), new ast::Identifier(L, "i"), new ast::Call(L, new ast::Identifier(L, "Assign"), { new ast::BinaryOperator(L,new ast::Identifier(L, "i"), new ast::Literal(L, 0.0)), new ast::Literal(L, 1.0) })});
	REQUIRE(*parse(tokenize(test1)) == *r1);
	delete r1;
}
	
TEST_CASE("Evaluation parsing", "[parser]")
{
	const char* test1 = "Human-\"EyeColor\"() # Returns \"blue\"";
	ast::Expression* r1 = new ast::Call(L, new ast::Identifier(L, "Object"), { new ast::Identifier(L,"Main"), new ast::Call(L, new ast::BinaryOperator(L, new ast::Identifier(L, "Human"), new ast::Literal(L, std::variant<double, std::string>("EyeColor"))), {})});
	REQUIRE(*parse(tokenize(test1)) == *r1);
	delete r1;

	const char* test2 = "CarArray-1() # Returns \"BMW\"";
	ast::Expression* r2 = new ast::Call(L, new ast::Identifier(L, "Object"), { new ast::Identifier(L,"Main"), new ast::Call(L, new ast::BinaryOperator(L, new ast::Identifier(L, "CarArray"), new ast::Literal(L, 1.0)), {}) });
	REQUIRE(*parse(tokenize(test2)) == *r2);
	delete r2;

	const char* test3 = "CountCars()()() # Insanity";
	ast::Expression* r3 = new ast::Call(L, new ast::Identifier(L, "Object"), { new ast::Identifier(L,"Main"), new ast::Call(L,new ast::Call(L, new ast::Call(L, new ast::Identifier(L, "CountCars"), {}), {}), {})});
	REQUIRE(*parse(tokenize(test3)) == *r3);
	delete r3;
	
	const char* test4 = "Human-EyeColor()";
	ast::Expression* r4 = new ast::Call(L, new ast::Identifier(L, "Object"), { new ast::Identifier(L,"Main"), new ast::BinaryOperator(L, new ast::Identifier(L, "Human"), new ast::Call(L, new ast::Identifier(L,"EyeColor"), {}))});
	REQUIRE(*parse(tokenize(test4)) == *r4);
	delete r4;
}

TEST_CASE("Nested members", "[parser]")
{
	const char* test1 = "obj-0-1";
	ast::Expression* r1 = new ast::Call(L, new ast::Identifier(L, "Object"), { new ast::Identifier(L,"Main"), new ast::BinaryOperator(L, new ast::BinaryOperator(L, new ast::Identifier(L, "obj"), new ast::Literal(L,0.0) ), new ast::Literal(L,1.0))});
	REQUIRE(*parse(tokenize(test1)) == *r1);
	delete r1;
}

TEST_CASE("Parsing complex nested calls", "[parser]")
{
	const char* test1 = "i \n Object( body, Print(param1), Assign(param1-0, Add(param1(), 1)) )\nWhile(SmallerThan(i, 5), body(i) )";
	ast::Expression* r1 = new ast::Call(L, new ast::Identifier(L, "Object"), { new ast::Identifier(L,"Main"),
		new ast::Identifier(L, "i"),
		new ast::Call(L, new ast::Identifier(L, "Object"), { new ast::Identifier(L, "body"), new ast::Call(L, new ast::Identifier(L, "Print"), {new ast::Identifier(L, "param1")}), new ast::Call(L, new ast::Identifier(L, "Assign"), {new ast::BinaryOperator(L, new ast::Identifier(L, "param1"),new ast::Literal(L, 0.0)), new ast::Call(L, new ast::Identifier(L, "Add"), {new ast::Call(L, new ast::Identifier(L, "param1"), {}), new ast::Literal(L, 1.0)}) })}),
		new ast::Call(L, new ast::Identifier(L, "While"), { new ast::Call(L, new ast::Identifier(L, "SmallerThan"), {new ast::Identifier(L, "i"), new ast::Literal(L, 5.0)}), new ast::Call(L, new ast::Identifier(L, "body"), {new ast::Identifier(L, "i")})})
		});
	REQUIRE(*parse(tokenize(test1)) == *r1);
	delete r1;
}

TEST_CASE("Numeric values", "[parser]")
{
	const char* test1 = "Add(1 -1)";
	ast::Expression* r1 = new ast::Call(L, new ast::Identifier(L, "Object"), { new ast::Identifier(L,"Main"), new ast::Call(L, new ast::Identifier(L,"Add"),{new ast::Literal(L,1.0) , new ast::Literal(L,-1.0)}) });
	REQUIRE(*parse(tokenize(test1)) == *r1);
	delete r1;
}