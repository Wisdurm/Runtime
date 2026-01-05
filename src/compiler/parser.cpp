#include "parser.h"
#include "tokenizer.h"
#include "exceptions.h"

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
	/// <param name="expected">(optional) The expected value of token.getText()</param>
	static Token consume(const std::vector<Token>& tokens, std::string expected = "")
	{
		auto token = peek(tokens);
		if (expected != "" and *token.getText() != expected) {
			throw ParserException("Unexpected token encountered", token.getSrc().getLine(), *token.getSrc().getFile());
		}
		pos++;
		return token;
	};

	static std::shared_ptr<ast::Expression> parseIdentifier(const std::vector<Token>& tokens);
	static std::shared_ptr<ast::Expression> parsePunctuation(const std::vector<Token>& tokens);
	static std::shared_ptr<ast::Expression> parseLiteral(const std::vector<Token>& tokens);
	static std::shared_ptr<ast::Expression> parseFunction(const std::vector<Token>& tokens, std::shared_ptr<ast::Expression> function);
	static std::shared_ptr<ast::Expression> parseBinaryLeft(const std::vector<Token>& tokens, std::shared_ptr<ast::Expression> left = nullptr);

	/// <summary>
	/// Main function for parsing an expression, will branch off to more specific functions
	/// </summary>
	/// <param name="parseSingle">Whether or not to parse - tokens</param>
	/// <param name="parsePure">If only pure expressions should be parsed (eq. no functions or binary operations). False by default</param>
	/// <returns>Ast tree</returns>
	static std::shared_ptr<ast::Expression> parseExpression(const std::vector<Token>& tokens, bool parsePure = false)
	{
		std::shared_ptr<ast::Expression> expr;
		switch (peek(tokens).getType())
		{
		case TokenType::IDENTIFIER:
		{
			expr = parseIdentifier(tokens);
			parsePure = false;
			break;
		}
		case TokenType::LITERAL:
		{
			expr = parseLiteral(tokens);
			break;
		}
		case TokenType::PUNCTUATION:
		{
			expr = parsePunctuation(tokens);
			return expr;
			// The only time code will reach here is if we have a negative value
			// and if we do, we know it's not going to need any of the otherwise
			// following checks
		}
		default:
			throw ParserException("Unhandled token type", expr->src.getLine(), expr->src.getFile()->c_str());
		}

		if (*peek(tokens).getText() == "-" and !parsePure)
		{
			if (std::dynamic_pointer_cast<ast::Identifier>(expr) != nullptr) // Member being accessed
				expr = parseBinaryLeft(tokens, expr);
			else return expr;
			// If last one is not identifier, then this one is surely a negative value
			// In order to parse the negative value, we need to first return the expression
		}
		while (*peek(tokens).getText() == "(" and !parsePure)
		{
			expr = parseFunction(tokens, expr);
		}
		return expr;
	}

	/// <summary>
	/// Parse identifier token
	/// </summary>
	/// <returns>Ast tree</returns>
	static std::shared_ptr<ast::Expression> parseIdentifier(const std::vector<Token>& tokens)
	{
		Token token = consume(tokens);
		return make_shared<ast::Identifier>(token.getSrc(), *token.getText());
	}
	
	/// <summary>
	/// Parse punctuation token
	/// </summary>
	/// <returns>Ast tree</returns>
	static std::shared_ptr<ast::Expression> parsePunctuation(const std::vector<Token>& tokens)
	{
		consume(tokens, "-");
		std::shared_ptr<ast::Literal> expr = std::dynamic_pointer_cast<ast::Literal>(parseExpression(tokens, true));
		if (expr == nullptr)
			throw ParserException("Expected literal after '-' token", expr->src.getLine(), expr->src.getFile()->c_str());
		
		// Invert value
		if (std::holds_alternative<double>(expr->litValue)) // Kind of weird to have this in the parser but it works :shrug:
			expr->litValue = std::get<double>(expr->litValue) * -1;
		else
			throw ParserException("You're mentally ill", expr->src.getLine(), expr->src.getFile()->c_str());
			// If you genuinely wrote -"1" in your code you don't deserve to have access to a computer

		return expr;
	}

	/// <summary>
	/// Parse literal token
	/// </summary>
	/// <returns>Ast tree</returns>
	static std::shared_ptr<ast::Expression> parseLiteral(const std::vector<Token>& tokens)
	{
		Token token = consume(tokens);
		if ((*token.getText())[0] == '\"')
		{	// String literal
			std::string stringValue = *token.getText();
			// Remove quatation marks
			stringValue.erase(0,1);
			stringValue.pop_back();
			return std::make_shared<ast::Literal>(token.getSrc(), std::variant<double, std::string>(stringValue)); // Using value constructor for clarity
		}
		else 
		{	// Number literal
			return std::make_shared<ast::Literal>(token.getSrc(), std::variant<double, std::string>(std::stod(*token.getText()) ));
		}
	}

	/// <summary>
	/// Parses binary operator with left assosiatety
	/// </summary>
	/// <returns></returns>
	static std::shared_ptr<ast::Expression> parseBinaryLeft(const std::vector<Token>& tokens, std::shared_ptr<ast::Expression> left)
	{
		while (*peek(tokens).getText() == "-")
		{
			consume(tokens, "-");
			auto right = parseExpression(tokens, true);
			left = std::make_shared<ast::BinaryOperator>(peek(tokens).getSrc(),
				left,
				right
			);
		}
		return left;
	}

	/// <summary>
	/// Parse function
	/// </summary>
	/// <returns>Ast tree</returns>
	static std::shared_ptr<ast::Expression> parseFunction(const std::vector<Token>& tokens, std::shared_ptr<ast::Expression> function)
	{
		auto beg = consume(tokens, "(");
		
		std::vector<std::shared_ptr<ast::Expression>> args;
		while (*peek(tokens).getText() != ")")
		{
			if (pos == tokens.size())
				throw ParserException("Unmatched parentheses, starting at", beg.getSrc().getLine(), beg.getSrc().getFile()->c_str());
			args.push_back(parseExpression(tokens));
		}
		consume(tokens, ")");
		return std::make_shared<ast::Call>(peek(tokens).getSrc(), function, args);
	}

	/// <summary>
	/// Parses top level statements as arguments to a main function
	/// </summary
	/// <returns>Ast tree</returns>
	static std::shared_ptr<ast::Expression> parseMain(const std::vector<Token>& tokens)
	{
		std::vector<std::shared_ptr<ast::Expression>> args;
		args.push_back(std::make_shared<ast::Identifier>(peek(tokens).getSrc(), "Main"));
		while (peek(tokens).getType() != TokenType::END)
		{
			args.push_back(parseExpression(tokens));
		}
		return std::make_shared<ast::Call>(peek(tokens).getSrc(), std::make_shared<ast::Identifier>(SourceLocation(), "Object"), args);
	}

	std::shared_ptr<ast::Expression> parse(const std::vector<Token>& tokens, bool requireMain)
	{
		pos = 0;
		if (requireMain) // True by default
		{
			if (*tokens[2].getText() == "Main") // Check for main function
				return parseExpression(tokens);
			else
				return parseMain(tokens);
		}
		else // Don't use main function. This will only accept a single statement. Used for live interpret
		{
			return parseExpression(tokens);
		}
	}
}

namespace ast
{
	bool Literal::compare(const Expression& other) const
	{
		// Don't know of a better way to do this at the moment but since this is just for running the tests 
		// I don't think speed matters that much
		try
		{
			return litValue == dynamic_cast<const Literal&>(other).litValue;
		}
		catch (std::bad_cast e)
		{
			return false;
		}
	}

	bool Identifier::compare(const Expression& other) const
	{
		try
		{
			return name == dynamic_cast<const Identifier&>(other).name;
		}
		catch (std::bad_cast e)
		{
			return false;
		}
	}

	bool Call::compare(const Expression& other) const
	{
		try
		{
			const Call& otherCall = dynamic_cast<const Call&>(other);
			if (*object == *otherCall.object and args.size() == otherCall.args.size())
			{
				// Now check all args
				for (int i = 0; i < args.size(); i++)
				{
					if (*args[i] != *otherCall.args[i])
						return false;
				}
				return true;
			}
			else
				return false;
		}
		catch (std::bad_cast e)
		{
			return false;
		}
	}

	bool BinaryOperator::compare(const Expression& other) const
	{
		try
		{
			const BinaryOperator& otherBinOp = dynamic_cast<const BinaryOperator&>(other);
			return *left == *otherBinOp.left and *right == *otherBinOp.right;
		}
		catch (std::bad_cast e)
		{
			return false;
		}
	}
}
