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

	static ast::Expression* parseIdentifier(const std::vector<Token>& tokens);
	static ast::Expression* parseLiteral(const std::vector<Token>& tokens);
	static ast::Expression* parsePunctuation(const std::vector<Token>& tokens);
	static ast::Expression* parseFunction(const std::vector<Token>& tokens, std::string functionName);

	/// <summary>
	/// Main function for parsing an expression, will branch off to more specific functions
	/// </summary>
	/// <returns>Ast tree</returns>
	static ast::Expression* parseExpression(const std::vector<Token>& tokens)
	{
		switch (peek(tokens).getType())
		{
		case TokenType::IDENTIFIER:
		{
			return parseIdentifier(tokens);
		}
		case TokenType::LITERAL:
		{
			return parseLiteral(tokens);
		}
		}
	}

	/// <summary>
	/// Parse identifier token
	/// </summary>
	/// <returns>Ast tree</returns>
	static ast::Expression* parseIdentifier(const std::vector<Token>& tokens)
	{
		Token token = consume(tokens);
		if (*peek(tokens).getText() == "(") // Identifier being called
			return parseFunction(tokens, *token.getText());
		else
		{
			ast::Expression* node = new ast::Identifier(token.getSrc(), *token.getText());

			if (*peek(tokens).getText() == "-") // Member being accessed
			{
				consume(tokens, "-");
				return new ast::BinaryOperator(node->src, node, parseExpression(tokens));
			}
			else
				return node;
		}
	}

	/// <summary>
	/// Parse literal token
	/// </summary>
	/// <returns>Ast tree</returns>
	static ast::Expression* parseLiteral(const std::vector<Token>& tokens)
	{
		Token token = consume(tokens);
		if ((*token.getText())[0] == '\"')
		{	// String literal
			std::string stringValue = *token.getText();
			// Remove quatation marks
			stringValue.erase(0,1);
			stringValue.pop_back();
			return new ast::Literal(token.getSrc(), ast::value(stringValue)); // Using value constructor for clarity
		}
		else if ((*token.getText()).find('.')) // Contains not implemented in MSVC yet :(
		{	// Decimal literal
			return new ast::Literal(token.getSrc(), ast::value(std::stod(*token.getText()) ));
		}
		else
		{	// Int literal
			ast::Expression* node = new ast::Literal(token.getSrc(), std::stoi(*token.getText()));
			if (*peek(tokens).getText() == "(") // Member value begin accessed
			{
				consume(tokens, "(");
				consume(tokens, ")");
				return new ast::UnaryOperator(token.getSrc(), node);
			}
			else if (*peek(tokens).getText() == "-") // Accessing deeper level member
			{
				consume(tokens, "-");
				return new ast::BinaryOperator(node->src, node, parseExpression(tokens));
			}
			else
				return node;
		}
	}

	/// <summary>
	/// Parse punctuation token
	/// </summary>
	/// <returns>Ast tree</returns>
	static ast::Expression* parsePunctuation(const std::vector<Token>& tokens)
	{

	}

	/// <summary>
	/// Parse function
	/// </summary>
	/// <returns>Ast tree</returns>
	static ast::Expression* parseFunction(const std::vector<Token>& tokens, std::string functionName)
	{
		consume(tokens, "(");
		std::vector<ast::Expression*> args;
		while (*peek(tokens).getText() != ")")
		{
			args.push_back(parseExpression(tokens));
		}
		consume(tokens, ")");
		return new ast::Call(peek(tokens).getSrc(), functionName, args);
	}

	ast::Expression* parse(const std::vector<Token>& tokens)
	{
		return parseExpression(tokens);
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
			return value == dynamic_cast<const Literal&>(other).value;
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
			if (name == otherCall.name and args.size() == otherCall.args.size())
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
			return left == otherBinOp.left and right == otherBinOp.right;
		}
		catch (std::bad_cast e)
		{
			return false;
		}
	}

	bool UnaryOperator::compare(const Expression& other) const
	{
		try
		{
			const UnaryOperator& otherUnOp = dynamic_cast<const UnaryOperator&>(other);
			return expr == otherUnOp.expr;
		}
		catch (std::bad_cast e)
		{
			return false;
		}
	}
}