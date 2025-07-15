#pragma once
#include <vector>
#include <string>

namespace rt
{
	enum class TokenType {
		IDENTIFIER,
		LITERAL,
		PUNCTUATION, // . and [] operators as well as parentheses
	};

	/// <summary>
	/// Class which defines the position of a line of code in the source code
	/// </summary>
	class SourceLocation
	{
	public:
		/// <summary>
		/// Default constructor
		/// </summary>
		SourceLocation(const int line, const std::string file) : line(line), file(file) {};

		// Getters
		
		/// <summary>
		/// Returns line member
		/// </summary>
		const int getLine() const { return line; };
		/// <summary>
		/// Returns file member
		/// </summary>
		const std::string* getFile() const { return &file; };

		// Operators

		//TODO: I have no idea how to do operator overloading

		friend bool operator==(const SourceLocation src1, const SourceLocation src2)
		{
			return src1.file == src2.file and src1.line == src2.line;
		};
	private:
		/// <summary>
		/// The line of the source code
		/// </summary>
		const int line;
		/// <summary>
		/// Source code file
		/// </summary>
		const std::string file;
	};

	class Token
	{
	public:
		/// <summary>
		/// Default constructor
		/// </summary>
		Token(const std::string text, const TokenType type, const SourceLocation location) : text(text), type(type), src(location) {};
		/// <summary>
		/// Constructor without specified source code location, the src member will default to line: -1 file: "\0"
		/// </summary>
		Token(const std::string text, const TokenType type) : text(text), type(type), src(SourceLocation(-1, "\0")) {};

		// Getters

		/// <summary>
		/// Return text member
		/// </summary>
		const std::string* getText() const { return &text; };
		/// <summary>
		/// Return type member
		/// </summary>
		const TokenType getType() const { return type; };
		/// <summary>
		/// Return src member
		/// </summary>
		const SourceLocation getSrc() const { return src; };

		// Operators

		//TODO: I have no idea how to do operator overloading

		friend bool operator==(const Token token1, const Token token2)
		{
			return token1.src == token2.src and token1.text == token2.text and token1.type == token2.type;
		};

		/// <summary>
		/// Punctuation which the tokenizer will look out for (operators, parentheses etc.)
		/// </summary>
		static const char punctuation[];
	private:
		/// <summary>
		/// The text of the token (C style string)
		/// </summary>
		const std::string text;
		/// <summary>
		/// The type of the token (C style string)
		/// </summary>
		const TokenType type;
		/// <summary>
		/// Location of the token within the source code
		/// </summary>
		const SourceLocation src;
	};

	/// <summary>
	/// Parses source code and returns a list tokens parsed from it
	/// </summary>
	/// <param name="src">Source code (C style string)</param>
	/// <returns>List of tokens parsed from source code</returns>
	std::vector<Token> tokenize(char* src);
}