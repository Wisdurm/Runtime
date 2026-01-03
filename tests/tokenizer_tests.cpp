// Catch 2
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_exception.hpp>
// Runtime
#include "../src/compiler/tokenizer.h"
// C++
#include <vector>
#include <iostream>

TEST_CASE("String tokenizing", "[token]")
{
	const char* test1 = "Import(StandardLibrary)\n# I can eat glass and it doesn't hurt me \nObject(\"Main\" Zero-0)\n";
	
	const std::vector<rt::Token> r1 = { rt::Token("Import", rt::TokenType::IDENTIFIER), rt::Token("(", rt::TokenType::PUNCTUATION), rt::Token("StandardLibrary", rt::TokenType::IDENTIFIER),
		rt::Token(")", rt::TokenType::PUNCTUATION), rt::Token("Object", rt::TokenType::IDENTIFIER), rt::Token("(", rt::TokenType::PUNCTUATION) ,rt::Token("\"Main\"", rt::TokenType::LITERAL),
		rt::Token("Zero", rt::TokenType::IDENTIFIER), rt::Token("-", rt::TokenType::PUNCTUATION), rt::Token("0", rt::TokenType::LITERAL), rt::Token(")", rt::TokenType::PUNCTUATION)
	};
	REQUIRE(rt::tokenize(test1) == r1);
}


TEST_CASE("Exceptions", "[token]")
{
	const char* test1 = "\"";
	REQUIRE_THROWS_WITH(rt::tokenize(test1), "Unmatched string literal");
}
