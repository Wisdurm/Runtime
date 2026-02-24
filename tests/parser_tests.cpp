// Catch 2
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_exception.hpp>
// Runtime
#include "../src/compiler/tokenizer.h"
#include "../src/compiler/parser.h"
// C++
#include <variant>

/// <summary>
/// Placeholder values for code locations
/// </summary>
static const rt::SourceLocation L;

using namespace rt;

TEST_CASE("Expression comparison", "[parser]")
{
	
	// Verify operator== works
	auto r1 = std::make_shared<ast::Call>(L, std::make_shared<ast::Identifier>(L, "Print"), std::vector<std::shared_ptr<ast::Expression>>{std::make_shared<ast::Literal>(L, 2.0), std::make_shared<ast::Identifier>(L, "asd")});
	auto r2 = std::make_shared<ast::Call>(L, std::make_shared<ast::Identifier>(L, "Print"), std::vector<std::shared_ptr<ast::Expression>>{ std::make_shared<ast::Literal>(L, 2.0), std::make_shared<ast::Identifier>(L, "asd")});
	REQUIRE(*r1 == *r2);
}

TEST_CASE("Literals", "[parser]")
{
	const char* test1 = "Object(Main, 1, 1.2, \"1.2\")";
	auto r1 = std::make_shared<ast::Call>(L, std::make_shared<ast::Identifier>(L, "Object"), std::vector<std::shared_ptr<ast::Expression>>{ std::make_shared<ast::Identifier>(L,"Main"), std::make_shared<ast::Literal>(L, 1.0), std::make_shared<ast::Literal>(L, 1.2) , std::make_shared<ast::Literal>(L, std::variant<double, std::string>("1.2")) });
	auto v = parse(tokenize(test1));
	REQUIRE(*v == *r1);
}

TEST_CASE("Top level statements", "[parser]")
{
	// Test basic top level statements
	const char* test1 = "i\nAssign(i-0,1)";
	auto r1 = std::make_shared<ast::Call>(L, std::make_shared<ast::Identifier>(L, "Object"), std::vector<std::shared_ptr<ast::Expression>>{ std::make_shared<ast::Identifier>(L,"Main"), std::make_shared<ast::Identifier>(L, "i"), std::make_shared<ast::Call>(L, std::make_shared<ast::Identifier>(L, "Assign"), std::vector<std::shared_ptr<ast::Expression>>{ std::make_shared<ast::BinaryOperator>(L,std::make_shared<ast::Identifier>(L, "i"), std::make_shared<ast::Literal>(L, 0.0)), std::make_shared<ast::Literal>(L, 1.0) })});
	REQUIRE(*parse(tokenize(test1)) == *r1);
}
	
TEST_CASE("Evaluation parsing", "[parser]")
{
	const char* test1 = "Human-\"EyeColor\"() # Returns \"blue\"";
	auto r1 = std::make_shared<ast::Call>(L, std::make_shared<ast::Identifier>(L, "Object"), std::vector<std::shared_ptr<ast::Expression>>{ std::make_shared<ast::Identifier>(L,"Main"), std::make_shared<ast::Call>(L, std::make_shared<ast::BinaryOperator>(L, std::make_shared<ast::Identifier>(L, "Human"), std::make_shared<ast::Literal>(L, std::variant<double, std::string>("EyeColor"))), std::vector<std::shared_ptr<ast::Expression>>{})});
	REQUIRE(*parse(tokenize(test1)) == *r1);

	const char* test2 = "CarArray-1() # Returns \"BMW\"";
	auto r2 = std::make_shared<ast::Call>(L, std::make_shared<ast::Identifier>(L, "Object"), std::vector<std::shared_ptr<ast::Expression>>{ std::make_shared<ast::Identifier>(L,"Main"), std::make_shared<ast::Call>(L, std::make_shared<ast::BinaryOperator>(L, std::make_shared<ast::Identifier>(L, "CarArray"), std::make_shared<ast::Literal>(L, 1.0)), std::vector<std::shared_ptr<ast::Expression>>{}) });
	REQUIRE(*parse(tokenize(test2)) == *r2);

	const char* test3 = "CountCars()()() # Insanity";
	auto r3 = std::make_shared<ast::Call>(L, std::make_shared<ast::Identifier>(L, "Object"), std::vector<std::shared_ptr<ast::Expression>>{ std::make_shared<ast::Identifier>(L,"Main"), std::make_shared<ast::Call>(L,std::make_shared<ast::Call>(L, std::make_shared<ast::Call>(L, std::make_shared<ast::Identifier>(L, "CountCars"), std::vector<std::shared_ptr<ast::Expression>>{}), std::vector<std::shared_ptr<ast::Expression>>{}), std::vector<std::shared_ptr<ast::Expression>>{})});
	REQUIRE(*parse(tokenize(test3)) == *r3);
	
	const char* test4 = "Human-EyeColor()";
	auto r4 = std::make_shared<ast::Call>(L, std::make_shared<ast::Identifier>(L, "Object"), std::vector<std::shared_ptr<ast::Expression>>{ std::make_shared<ast::Identifier>(L,"Main"), std::make_shared<ast::BinaryOperator>(L, std::make_shared<ast::Identifier>(L, "Human"), std::make_shared<ast::Call>(L, std::make_shared<ast::Identifier>(L,"EyeColor"), std::vector<std::shared_ptr<ast::Expression>>{}))});
	REQUIRE(*parse(tokenize(test4)) == *r4);
}

TEST_CASE("Nested members", "[parser]")
{
	const char* test1 = "obj-0-1";
	auto r1 = std::make_shared<ast::Call>(L, std::make_shared<ast::Identifier>(L, "Object"), std::vector<std::shared_ptr<ast::Expression>>{ std::make_shared<ast::Identifier>(L,"Main"), std::make_shared<ast::BinaryOperator>(L, std::make_shared<ast::BinaryOperator>(L, std::make_shared<ast::Identifier>(L, "obj"), std::make_shared<ast::Literal>(L,0.0) ), std::make_shared<ast::Literal>(L,1.0))});
	REQUIRE(*parse(tokenize(test1)) == *r1);
}

TEST_CASE("Parsing complex nested calls", "[parser]")
{
	const char* test1 = "i \n Object( body, Print(param1), Assign(param1-0, Add(param1(), 1)) )\nWhile(SmallerThan(i, 5), body(i) )";
	auto r1 = std::make_shared<ast::Call>(L, std::make_shared<ast::Identifier>(L, "Object"), std::vector<std::shared_ptr<ast::Expression>>{ std::make_shared<ast::Identifier>(L,"Main"),
		std::make_shared<ast::Identifier>(L, "i"),
		std::make_shared<ast::Call>(L, std::make_shared<ast::Identifier>(L, "Object"), std::vector<std::shared_ptr<ast::Expression>>{ std::make_shared<ast::Identifier>(L, "body"), std::make_shared<ast::Call>(L, std::make_shared<ast::Identifier>(L, "Print"), std::vector<std::shared_ptr<ast::Expression>>{std::make_shared<ast::Identifier>(L, "param1")}), std::make_shared<ast::Call>(L, std::make_shared<ast::Identifier>(L, "Assign"), std::vector<std::shared_ptr<ast::Expression>>{std::make_shared<ast::BinaryOperator>(L, std::make_shared<ast::Identifier>(L, "param1"),std::make_shared<ast::Literal>(L, 0.0)), std::make_shared<ast::Call>(L, std::make_shared<ast::Identifier>(L, "Add"), std::vector<std::shared_ptr<ast::Expression>>{std::make_shared<ast::Call>(L, std::make_shared<ast::Identifier>(L, "param1"), std::vector<std::shared_ptr<ast::Expression>>{}), std::make_shared<ast::Literal>(L, 1.0)}) })}),
		std::make_shared<ast::Call>(L, std::make_shared<ast::Identifier>(L, "While"), std::vector<std::shared_ptr<ast::Expression>>{ std::make_shared<ast::Call>(L, std::make_shared<ast::Identifier>(L, "SmallerThan"), std::vector<std::shared_ptr<ast::Expression>>{std::make_shared<ast::Identifier>(L, "i"), std::make_shared<ast::Literal>(L, 5.0)}), std::make_shared<ast::Call>(L, std::make_shared<ast::Identifier>(L, "body"), std::vector<std::shared_ptr<ast::Expression>>{std::make_shared<ast::Identifier>(L, "i")})})
		});
	REQUIRE(*parse(tokenize(test1)) == *r1);
}

TEST_CASE("Numeric values", "[parser]")
{
	const char* test1 = "Add(1 -1)";
	auto r1 = std::make_shared<ast::Call>(L, std::make_shared<ast::Identifier>(L, "Object"), std::vector<std::shared_ptr<ast::Expression>>{ std::make_shared<ast::Identifier>(L,"Main"), std::make_shared<ast::Call>(L, std::make_shared<ast::Identifier>(L,"Add"),std::vector<std::shared_ptr<ast::Expression>>{std::make_shared<ast::Literal>(L,1.0) , std::make_shared<ast::Literal>(L,-1.0)}) });
	REQUIRE(*parse(tokenize(test1)) == *r1);
}

TEST_CASE("Exceptions", "[parser]")
{
	const char* test1 = "-\"1\"";
	REQUIRE_THROWS_WITH(parse(tokenize(test1)),"You're mentally ill");

	const char* test2 = "Print(";
	REQUIRE_THROWS_WITH(parse(tokenize(test2)),"Unmatched parentheses, starting at");

	const char* test3 = "a-(";
	REQUIRE_THROWS_WITH(parse(tokenize(test3)),"Unexpected token encountered");
}
