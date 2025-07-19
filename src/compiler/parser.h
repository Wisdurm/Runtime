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
	/// <returns>An ast tree representing the tokens</returns>
	ast::Expression* parse(const std::vector<Token>& tokens);
}