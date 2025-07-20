#pragma once

#include "../src/compiler/tokenizer.h"
// C++
#include <cassert>
#include <vector>
#include <iostream>

namespace rt
{
	/// <summary>
	/// Test basic function of the tokenizer
	/// </summary>
	void test_tokenizer_basics()
	{
		std::cout << "Testing tokenizer basics..." << std::endl;
		const char* test1 = "Import(StandardLibrary)\n# I can eat glass and it doesn't hurt me \nObject(\"Main\", Zero-0)\n";
		std::vector<Token> r1 = { Token("Import", TokenType::IDENTIFIER), Token("(", TokenType::PUNCTUATION), Token("StandardLibrary", TokenType::IDENTIFIER),
			Token(")", TokenType::PUNCTUATION), Token("Object", TokenType::IDENTIFIER), Token("(", TokenType::PUNCTUATION) ,Token("\"Main\"", TokenType::LITERAL),
			Token(",", TokenType::PUNCTUATION), Token("Zero", TokenType::IDENTIFIER), Token("-", TokenType::PUNCTUATION), Token("0", TokenType::LITERAL), Token(")", TokenType::PUNCTUATION) };
		assert(tokenize(test1) == r1);
		std::cout << "Passed tokenizer basics!" << std::endl;
	}
}