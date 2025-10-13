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
	std::vector<Token> tokenize(const char* src, const char* srcFile)
	{
		// TODO: use std::string
		int line = 0;

		std::vector<Token> tokens;

		// Parse src code
		const int srcLen = strlen(src);
		for (int srcI = 0; srcI < srcLen; )
		{
			// Advance forward in the text, while also keeping the source locations up to date
			auto advance = [&srcI, &line, src]() {
				srcI++;
				if (src[srcI] == '\n')
					line++;
			};

			// Whether or not any patterns have been matched. If none have been, iterate to prevent infinite loop
			bool match = false;

			// First check for comment
			if (src[srcI] == '#')
			{
				match = true;
				do // Skip until newline
				{
					advance();
				} while (src[srcI] != '\n');
			}
			// Check for identifier
			if (isalnum(src[srcI]) and not isdigit(src[srcI])) // Identifier can't begin with a number
			{
				match = true;
				std::string identifier;
				do {
					identifier += src[srcI];
					advance();
				} while (isalnum(src[srcI]));
				tokens.push_back(Token(identifier, TokenType::IDENTIFIER, SourceLocation(line, srcFile)));
			}
			// Check for int literal
			if (isdigit(src[srcI]))
			{
				match = true;
				std::string intLiteral;
				do {
					intLiteral += src[srcI];
					advance();
				} while (isdigit(src[srcI]) or src[srcI] == '.');
				tokens.push_back(Token(intLiteral, TokenType::LITERAL, SourceLocation(line, srcFile)));
			}		
			// Check for string literal
			if (src[srcI] == '\"')
			{
				match = true;
				std::string stringLiteral;
				do {
					stringLiteral += src[srcI];
					advance();
					if (srcI > srcLen)
					{
						throw;
					}
				} while (src[srcI] != '\"'); // Go until the end of the string literal
				stringLiteral += '\"';
				advance();
				tokens.push_back(Token(stringLiteral, TokenType::LITERAL, SourceLocation(line, srcFile)));
			}
			// Check for punctuation
			for (char punc : Token::punctuation)
			{
				if (src[srcI] == punc)
				{
					match = true;
					std::string character;
					character += src[srcI];
					tokens.push_back(Token( character, TokenType::PUNCTUATION, SourceLocation(line, srcFile)));
					advance();
				}
			}

			if (not match)
				advance();
		}
		// Return result
		return tokens;
	}
}