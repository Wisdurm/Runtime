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
		char test1[] = "Import(StandardLibrary)\n# I can eat glass and it doesn't hurt me \nObject(\"Main\", Zero)\n";
		std::vector<Token> r1 = { Token("Import", TokenType::IDENTIFIER), Token("(", TokenType::PUNCTUATION), Token("StandardLibrary", TokenType::IDENTIFIER),
			Token(")", TokenType::PUNCTUATION), Token("Object", TokenType::IDENTIFIER), Token("(", TokenType::PUNCTUATION) ,Token("\"Main\"", TokenType::LITERAL),
			Token(",", TokenType::PUNCTUATION), Token("Zero", TokenType::IDENTIFIER), Token(")", TokenType::PUNCTUATION) };
		assert(tokenize(test1) == r1);
	}
}