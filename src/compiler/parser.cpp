#include "parser.h"
#include "tokenizer.h"

namespace rt
{
	/// <summary>
	/// Current position on the token list
	/// </summary>
	static int pos = 0;

	/// <summary>
	/// Returns current token
	/// </summary>
	static Token peek(const std::vector<Token> &tokens)
	{
		if (pos < tokens.size())
			return tokens[pos];
		else
			return Token("end", TokenType::END);
	};

	/// <summary>
	/// Returns current token and moves the position forward by one
	/// </summary>
	static Token consume(const std::vector<Token>& tokens)
	{
		auto token = peek(tokens);
		pos++;
		return token;
	};

	/// <summary>
	/// Main function for parsing an expression, will branch off to more specific functions
	/// </summary>
	/// <returns></returns>
	static ast::Expression parseExpression(const std::vector<Token>& tokens)
	{
		return ast::Expression();
	}

	ast::Expression parse(const std::vector<Token>& tokens)
	{
		return parseExpression(tokens);
	}
}

namespace ast
{
	bool ast::Expression::compare(const Expression ast1, const Expression ast2)
	{
		if (dynamic_cast<const Literal*>(&ast1) != nullptr and dynamic_cast<const Literal*>(&ast2) != nullptr)
		{

			auto l1 = dynamic_cast<const Literal*>(&ast1);
			auto l2 = dynamic_cast<const Literal*>(&ast2);
			return l1->value == l2->value;
		}
		else if (dynamic_cast<const Identifier*>(&ast1) != nullptr and dynamic_cast<const Identifier*>(&ast2) != nullptr)
		{
			auto i1 = dynamic_cast<const Identifier*>(&ast1);
			auto i2 = dynamic_cast<const Identifier*>(&ast2);
			return i1->name == i2->name;
		}
		else if (dynamic_cast<const Call*>(&ast1) != nullptr and dynamic_cast<const Call*>(&ast2) != nullptr)
		{
			auto c1 = dynamic_cast<const Call*>(&ast1);
			auto c2 = dynamic_cast<const Call*>(&ast2);
			if (c1->name == c2->name and c1->argAmount == c2->argAmount)
			{
				for (int i = 0; i < c1->argAmount; i++)
				{
					if (!compare(c1->args[i], c2->args[i]))
						return false;
				}
				return true;
			}
			else
				return false;
		}
	}
}