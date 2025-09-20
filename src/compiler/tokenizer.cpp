#include "tokenizer.h"
// C++
#include <vector>
#include <string>
// C
#include <string.h>

namespace rt
{
	const char Token::punctuation[] = {'-','(', ')', ',', '<', '>'};
	
	// Tokenizer
	std::vector<Token> tokenize(const char* src)
	{
		//TODO, use std::string
		// TODO: Implement srcLoc at later point
		SourceLocation srcLoc = SourceLocation(-1, "\0");

		std::vector<Token> tokens;
		// Parse src code
		const int srcLen = strlen(src);
		for (int srcI = 0; srcI < srcLen; )
		{
			// Whether or not any patterns have been matched. If none have been, iterate to prevent infinite loop
			bool match = false;

			// First check for comment
			if (src[srcI] == '#')
			{
				match = true;
				do // Skip until newline
				{
					srcI++;
				} while (src[srcI] != '\n');
			}
			// Check for identifier
			if (isalnum(src[srcI]) and not isdigit(src[srcI])) // Identifier can't begin with a number
			{
				match = true;
				std::string identifier;
				do {
					identifier += src[srcI];
					srcI++;
				} while (isalnum(src[srcI]));
				tokens.push_back(Token(identifier, TokenType::IDENTIFIER, srcLoc));
			}
			// Check for int literal
			if (isdigit(src[srcI]))
			{
				match = true;
				std::string intLiteral;
				do {
					intLiteral += src[srcI];
					srcI++;
				} while (isdigit(src[srcI]) or src[srcI] == '.');
				tokens.push_back(Token(intLiteral, TokenType::LITERAL, srcLoc));
			}		
			// Check for string literal
			if (src[srcI] == '\"')
			{
				match = true;
				std::string stringLiteral;
				do {
					stringLiteral += src[srcI];
					srcI++;
					if (srcI > srcLen)
					{
						throw;
					}
				} while (src[srcI] != '\"'); // Go until the end of the string literal
				stringLiteral += '\"';
				srcI++;
				tokens.push_back(Token(stringLiteral, TokenType::LITERAL, srcLoc));
			}
			// Check for punctuation
			for (char punc : Token::punctuation)
			{
				if (src[srcI] == punc)
				{
					match = true;
					std::string character;
					character += src[srcI];
					tokens.push_back(Token( character, TokenType::PUNCTUATION, srcLoc));
					srcI++;
				}
			}

			if (not match)
				srcI++;
		}
		// Return result
		return tokens;
	}
}