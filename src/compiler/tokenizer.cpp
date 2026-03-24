#include "tokenizer.h"
#include "exceptions.h"
// C++
#include <vector>
#include <string>
#include <algorithm>
// C
#include <string.h>

namespace rt
{
	const char Token::punctuation[] = {'-','(', ')'};
	const char Token::npunctuation[] = {',', '.'};
	const char Token::identchars[] = "+*/<=>^"; // TODO: Might be expanded later
	const char* li = "live-input";
	
	// Tokenizer
	std::vector<Token> tokenize(const char* src, const char* srcFile)
	{
		int line = 1;
		std::vector<Token> tokens;

		// Parse src code
		const int srcLen = strlen(src);
		for (int srcI = 0; srcI < srcLen; )
		{
			// Advance forward in the text, while also keeping the source locations up to date
			const auto advance = [&srcI, &line, src]() {
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
			// Check for number literal
			if (isdigit(src[srcI]))
			{
				match = true;
				std::string numberLiteral;
				do {
					numberLiteral += src[srcI];
					advance();
				} while (isdigit(src[srcI]) or src[srcI] == '.' or src[srcI] == ',');
				// Check for trailing decimal seperators
				if (src[srcI - 1] == '.' or src[srcI - 1] == ',') {
					throw TokenizerException("Number literal may not end with trailing decimal seperators", line-1, srcFile);
				}
				tokens.push_back(Token(numberLiteral, TokenType::NUMBER, SourceLocation(line, srcFile)));
			}		
			// Check for string literal
			if (src[srcI] == '\"' or src[srcI] == '\'')
			{
				advance();
				match = true;
				std::string stringLiteral;
				do {
					stringLiteral += src[srcI];
					advance();
					if (srcI > srcLen)
					{
						if (strcmp(srcFile, li) == 0)
							throw TokenizerException("Unmatched string literal", line, srcFile);
						else // Bruhhh
							throw TokenizerException("Unmatched string literal", line-1, srcFile);
					}
				} while (not((src[srcI] == '\"' or src[srcI] == '\'')
					     and src[srcI-1] != '\\'));
				// Go until the end of the string literal
				advance();
				tokens.push_back(Token(stringLiteral, TokenType::STRING, SourceLocation(line, srcFile)));
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
			// Check for unallowed punctuation (decimal seperators, which could get confusing otherwise)
			for (char npunc : Token::npunctuation)
			{
				if (src[srcI] == npunc)
				{
					throw TokenizerException("Decimal seperators not allowed for generic formatting", line-1, srcFile);
				}
			}
			// Only check for identifers as the very last option
			if (match)
				continue;
			// Check for identifier			
			if (isalnum(src[srcI]) or
			    std::find(std::begin(Token::identchars), std::end(Token::identchars), src[srcI]) != std::end(Token::identchars))
			{
				match = true;
				std::string identifier;
				do {
					identifier += src[srcI];
					advance();
				} while (isalnum(src[srcI]));
				tokens.push_back(Token(identifier, TokenType::IDENTIFIER, SourceLocation(line, srcFile)));
			}
			// Prevent infinite loop
			if (not match)
				advance();
		}
		// Return result
		return tokens;
	}
}
