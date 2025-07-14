#include "tokenizer.h"
// C++
#include <vector>
// C
#include <string.h>

namespace rt
{
	// Tokenizer

	std::vector<Token>* tokenize(char* src)
	{
		std::vector<Token> tokens;
		// Parse src code
		const int srcLen = strlen(src);
		for (int srcI = 0; srcI < srcLen; srcI++)
		{
			// First check for comment
			if (src[srcI] == '#')
			{
				do // Skip until newline
				{
					srcI++;
				} while (src[srcI] != '\n');
			}
			// Then loop through the rest of the regex
			
		}
		// Return result
		return &tokens;
	}
}