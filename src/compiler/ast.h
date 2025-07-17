#pragma once
#include "tokenizer.h"
// C++
#include <memory>
// Abstract syntax tree classes

#include <iostream>

namespace ast
{
	enum class valueType {
		INT,
		DEC,
		STR,
		REF
	};
	class value { // Tried to do this with union but it was so difficult like genuinely i dont understand anything
	public:
		/// <summary>
		/// Int constructor
		/// </summary>
		value(int value) : valueInt(value), type(valueType::INT) {};
		/// <summary>
		/// Decimal constructor
		/// </summary>
		value(double value) : valueDec(value), type(valueType::DEC) {};
		/// <summary>
		/// String constructor
		/// </summary>
		value(std::string value) : valueStr(value), type(valueType::STR) {};
	private:
		/// <summary>
		/// Current type of value
		/// </summary>
		const valueType type;
		/// <summary>
		/// Integer value
		/// </summary>
		long valueInt; // Might as well be long since we have string here
		/// <summary>
		/// Decimal value
		/// </summary>
		double valueDec; // Might as well be double
		/// <summary>
		/// String value
		/// </summary>
		std::string valueStr;

		// Operators

		//TODO: I have no idea how to do operator overloading

		friend bool operator==(const value v1, const value v2)
		{
			if (v1.type == v2.type)
			{
				switch (v1.type)
				{
				case valueType::INT:
				{
					return v1.valueInt == v2.valueInt;
					break;
				}
				case valueType::DEC:
				{
					return v1.valueDec == v2.valueDec;
					break;
				}
				case valueType::STR:
				{
					return v1.valueStr == v2.valueStr;
					break;
				}
				}
			}
			else
				return false;
		};
	};

	/// <summary>
	/// Base class for ast expression
	/// </summary>
	class Expression
	{
	public:
		virtual ~Expression() {}; // Virtual desctructor so dynamic cast works

		/// <summary>
		/// Default constructor
		/// </summary>
		/// <param name="src"></param>
		Expression(const rt::SourceLocation src) : src(src) {};
		/// <summary>
		/// Constructor without specified source code location, the src member will default to line: -1 file: "\0"
		/// </summary>
		/// <param name="src"></param>
		Expression() : src(rt::SourceLocation()) {};

		/// <summary>
		/// Source code location of ast expression
		/// </summary>
		const rt::SourceLocation src;

		// Operators

		//TODO: I have no idea how to do operator overloading

		friend bool operator==(const Expression ast1, const Expression ast2)
		{
			return compare(ast1, ast2);
		};
	private:
		/// <summary>
		/// Compare two ast trees
		/// </summary>
		/// <param name="ast1">Ast tree 1</param>
		/// <param name="ast2">Ast tree 2</param>
		/// <returns>Whether or not the trees are identical</returns>
		static bool compare(const Expression ast1, const Expression ast2);
	};

	/// <summary>
	/// Ast literal
	/// </summary>
	class Literal : public Expression
	{
	public:
		/// <summary>
		/// Default constructor
		/// </summary>
		Literal(const rt::SourceLocation src, const value value) : value(value), Expression(src) {};

		/// <summary>
		/// Value of literal
		/// </summary>
		const value value;
	};

	/// <summary>
	/// Ast class for identifier
	/// </summary>
	class Identifier : public Expression
	{
	public:
		/// <summary>
		/// Default constructor
		/// </summary>
		Identifier(const rt::SourceLocation src, const std::string name) : name(name), Expression(src) {};

		/// <summary>
		/// Name of identifier
		/// </summary>
		const std::string name;
	};

	/// <summary>
	/// Ast expression for calling an object
	/// </summary>
	class Call : public Expression
	{
	public:
		/// <summary>
		/// Default constructor
		/// </summary>
		Call(const rt::SourceLocation src, const std::string name, const Expression* args, const int argAmount) : name(name), args(args), argAmount(argAmount), Expression(src) {};

		/// <summary>
		/// Name of object to call
		/// </summary>
		const std::string name;
		/// <summary>
		/// Arguments to call object with
		/// </summary>
		const Expression* args;
		/// <summary>
		/// Amount of arguments
		/// </summary>
		const int argAmount;
	};


}