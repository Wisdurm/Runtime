#pragma once
#include "ast.h"
// C++
#include <vector>

namespace rt
{
	/// <summary>
	/// Parses a list of tokens
	/// </summary>
	/// <param name="tokens">List of tokens (presumably returned by the tokenize function)</param>
	/// <param name="requireMain">Whether or not to require a main function. 
	/// Without one, only a single statement can be parsed at a time. Set to true by default</param>
	/// <returns>An ast tree representing the tokens</returns>
	std::unique_ptr<ast::Expression> parse(const std::vector<Token>& tokens, bool requireMain = true);
}
