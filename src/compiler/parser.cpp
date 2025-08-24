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
	/// <param name="expected">(optional) The expected value of token.getText()</param>
	static Token consume(const std::vector<Token>& tokens, std::string expected = "")
	{
		auto token = peek(tokens);
		if (expected != "" and *token.getText() != expected)
			throw;
		pos++;
		return token;
	};

	static ast::Expression* parseIdentifier(const std::vector<Token>& tokens, const bool parseSingle);
	static ast::Expression* parseLiteral(const std::vector<Token>& tokens, const bool parseSingle);
	static ast::Expression* parseFunction(const std::vector<Token>& tokens, ast::Expression* function);
	static ast::Expression* parseBinaryLeft(const std::vector<Token>& tokens, ast::Expression* left = nullptr);

	/// <summary>
	/// Main function for parsing an expression, will branch off to more specific functions
	/// </summary>
	/// <param name="parseSingle">Whether or not to parse - tokens</param>
	/// <returns>Ast tree</returns>
	static ast::Expression* parseExpression(const std::vector<Token>& tokens, const bool parseSingle)
	{
		ast::Expression* expr;
		switch (peek(tokens).getType())
		{
		case TokenType::IDENTIFIER:
		{
			expr = parseIdentifier(tokens, parseSingle);
			break;
		}
		case TokenType::LITERAL:
		{
			expr = parseLiteral(tokens, parseSingle);
			break;
		}
		default:
			throw;
		}
		ast::Expression* r = expr;
		while (*peek(tokens).getText() == "(" and not parseSingle)
		{
			r = parseFunction(tokens, r);
		}
		return r;
	}

	/// <summary>
	/// Parse identifier token
	/// </summary>
	/// <returns>Ast tree</returns>
	static ast::Expression* parseIdentifier(const std::vector<Token>& tokens, const bool parseSingle)
	{
		Token token = consume(tokens);
		ast::Expression* node = new ast::Identifier(token.getSrc(), *token.getText());

		if (*peek(tokens).getText() == "-" and !parseSingle) // Member being accessed
		{
			return parseBinaryLeft(tokens, node);
		}
		else
			return node;
	}

	/// <summary>
	/// Parse literal token
	/// </summary>
	/// <returns>Ast tree</returns>
	static ast::Expression* parseLiteral(const std::vector<Token>& tokens, const bool parseSingle)
	{
		Token token = consume(tokens);
		if ((*token.getText()).find('.') != std::string::npos) // Contains not implemented in MSVC yet :(
		{	// Decimal literal
			return new ast::Literal(token.getSrc(), ast::value(std::stod(*token.getText()) ));
		}

		ast::Expression* node;

		if ((*token.getText())[0] == '\"')
		{	// String literal
			std::string stringValue = *token.getText();
			// Remove quatation marks
			stringValue.erase(0,1);
			stringValue.pop_back();
			node = new ast::Literal(token.getSrc(), ast::value(stringValue)); // Using value constructor for clarity
		}
		else
		{	// Int literal
			node = new ast::Literal(token.getSrc(), std::stoi(*token.getText()));
		}

		if (*peek(tokens).getText() == "-" and !parseSingle) // Accessing deeper level member
		{
			return parseBinaryLeft(tokens, node);
		}
		else
			return node;
	}

	/// <summary>
	/// Parses binary operator with left assosiatety
	/// </summary>
	/// <returns></returns>
	static ast::Expression* parseBinaryLeft(const std::vector<Token>& tokens, ast::Expression* left)
	{
		while (*peek(tokens).getText() == "-")
		{
			consume(tokens, "-");
			auto right = parseExpression(tokens, true);
			left = new ast::BinaryOperator(peek(tokens).getSrc(),
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
	static ast::Expression* parseFunction(const std::vector<Token>& tokens, ast::Expression* function)
	{
		consume(tokens, "(");
		std::vector<ast::Expression*> args;
		while (*peek(tokens).getText() != ")")
		{
			args.push_back(parseExpression(tokens, false));
			if (*peek(tokens).getText() == ",")
				consume(tokens, ",");
		}
		consume(tokens, ")");
		return new ast::Call(peek(tokens).getSrc(), function, args);
	}

	/// <summary>
	/// Parses top level statements as arguments to a main function
	/// </summary
	/// <returns>Ast tree</returns>
	static ast::Expression* parseMain(const std::vector<Token>& tokens)
	{
		std::vector<ast::Expression*> args;
		args.push_back(new ast::Identifier(peek(tokens).getSrc(), "Main"));
		while (peek(tokens).getType() != TokenType::END)
		{
			args.push_back(parseExpression(tokens, false));
		}
		return new ast::Call(peek(tokens).getSrc(), new ast::Identifier(SourceLocation(), "Object"), args);
	}

	ast::Expression* parse(const std::vector<Token>& tokens, bool requireMain)
	{
		pos = 0;
		if (requireMain) // True by default
		{
			if (*tokens[3].getText() == "Main") // Check for main function
				return parseExpression(tokens, false);
			else
				return parseMain(tokens);
		}
		else // Don't use main function. This will only accept a single statement
		{
			return parseExpression(tokens, false);
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