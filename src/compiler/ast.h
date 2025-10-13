#pragma once
#include "tokenizer.h"
// C++
#include <memory>
#include <vector>
#include <variant>
// Abstract syntax tree classes

// Debugging
#include <iostream>
#include <cassert>

namespace ast
{
	enum class valueType { // TODO : maybe obsolete? REF type still valuable
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
		value(int value) : valueHeld(value), type(valueType::INT) {};
		/// <summary>
		/// Decimal constructor
		/// </summary>
		value(double value) : valueHeld(value), type(valueType::DEC) {};
		/// <summary>
		/// String constructor
		/// </summary>
		value(std::string value) : valueHeld(value), type(valueType::STR) {};

		// TODO: Encapsulation

		/// <summary>
		/// Current type of value
		/// </summary>
		valueType type;
		/// <summary>
		/// Value of the class
		/// </summary>
		std::variant<long, double, std::string> valueHeld;

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
					return v1.valueHeld == v2.valueHeld;
					break;
				}
				case valueType::DEC:
				{
					return v1.valueHeld == v2.valueHeld;
					break;
				}
				case valueType::STR:
				{
					return v1.valueHeld == v2.valueHeld;
					break;
				}
				default:
					throw;
				}
			}
			else
				return false;
		}
		value& operator=(const value newValue)
		{
			valueHeld = newValue.valueHeld;
			type = newValue.type;
			return *this;
		}
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

		bool operator==(const Expression& other) const
		{
			return compare(other);
		};
	private:
		/// <summary>
		/// Compare two ast trees
		/// </summary>
		/// <param name="other">Ast node to compare against</param>
		/// <returns>Whether or not the trees are identical</returns>
		virtual bool compare(const Expression& other) const = 0;
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
		Literal(const rt::SourceLocation src, const value value) : litValue(value), Expression(src) {};

		/// <summary>
		/// Value of literal
		/// </summary>
		value litValue;
	private:
		/// <summary>
		/// Compare two ast trees
		/// </summary>
		/// <param name="other">Ast node to compare against</param>
		/// <returns>Whether or not the nodes are identical</returns>
		bool compare(const Expression& other) const override;
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
	private:
		/// <summary>
		/// Compare two ast trees
		/// </summary>
		/// <param name="other">Ast node to compare against</param>
		/// <returns>Whether or not the nodes are identical</returns>
		bool compare(const Expression& other) const override;
	};

	/// <summary>
	/// Ast expression for calling an object
	/// </summary>
	class Call : public Expression
	{
	public:
		~Call()
		{
			// Object
			delete object;
			// Args
			for (int i = 0; i < args.size(); i++)
				delete args[i];
		}

		/// <summary>
		/// Default constructor
		/// </summary>
		Call(const rt::SourceLocation src, Expression* object, const std::vector<Expression*> args) : object(object), args(args), Expression(src) {};
		/// <summary>
		/// Object to call
		/// </summary>
		Expression* object;
		/// <summary>
		/// Arguments to call object with. Array of pointers because Expression is an abstract class
		/// </summary>
		std::vector<Expression*> args; // I've spent hours on this
	private:
		/// <summary>
		/// Compare two ast trees
		/// </summary>
		/// <param name="other">Ast node to compare against</param>
		/// <returns>Whether or not the nodes are identical</returns>
		bool compare(const Expression& other) const override;
	};

	/// <summary>
	/// Ast expression for a binary operator (e.g. - )
	/// </summary>
	class BinaryOperator : public Expression
	{
	public:
		~BinaryOperator()
		{
			delete left;
			delete right;
		}

		/// <summary>
		/// Default constructor
		/// </summary>
		BinaryOperator(const rt::SourceLocation src, Expression* left, Expression* right) : left(left), right(right), Expression(src) {};
		/// <summary>
		/// Left expression
		/// </summary>
		Expression* left;
		/// <summary>
		/// Right expression
		/// </summary>
		Expression* right;
	private:
		/// <summary>
		/// Compare two ast trees
		/// </summary>
		/// <param name="other">Ast node to compare against</param>
		/// <returns>Whether or not the nodes are identical</returns>
		bool compare(const Expression& other) const override;
	};
}