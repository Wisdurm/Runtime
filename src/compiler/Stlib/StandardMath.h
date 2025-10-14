#pragma once
#include "StandardFiles.h"
#include "../interpreter.h"
#include "../parser.h"
// C++
#include <cmath>
#include <vector>
#include <variant>

// Automatically make single argument functions from C++ functions
#define SINGLE_ARG_FUNCTION(func, name) objectOrValue name(std::vector<objectOrValue>& args, SymbolTable* symtab, ArgState& argState) \
	{ if (args.size() > 0) \
		return func(getNumericalValue(VALUEHELD(args.at(0))));	\
		return Zero; }

// Same thing but two args
#define DOUBLE_ARG_FUNCTION(func, name) objectOrValue name(std::vector<objectOrValue>& args, SymbolTable* symtab, ArgState& argState) \
	{ if (args.size() > 1) \
		return func(getNumericalValue(VALUEHELD(args.at(0))), getNumericalValue(VALUEHELD(args.at(1))));	\
		return Zero; }

namespace rt
{
	/// <summary>
	/// Adds the value of all args together and retuns the sum
	/// </summary>
	/// <returns>Sum of all args</returns>
	objectOrValue Add(std::vector<objectOrValue>& args, SymbolTable* symtab, ArgState& argState)
	{
		double sum = 0;
		for (objectOrValue arg : args)
		{
			auto valueHeld VALUEHELD(arg);
			sum += getNumericalValue(valueHeld);
		}
		return ast::value(sum);
	}
	/// <summary>
	/// Negates the value of all args
	/// </summary>
	/// <returns></returns>
	objectOrValue Minus(std::vector<objectOrValue>& args, SymbolTable* symtab, ArgState& argState)
	{
		double result = 0;
		bool defined = false;
		for (objectOrValue arg : args)
		{
			auto valueHeld VALUEHELD(arg);
			if (!defined)
			{
				result = getNumericalValue(valueHeld);
				defined = true;
			}
			else
			{
				result -= getNumericalValue(valueHeld);
			}
		}
		return ast::value(result);
	}
	/// <summary>
	/// Multiplies the value of all args and returns the result
	/// </summary>
	/// <returns>Sum of all args</returns>
	objectOrValue Multiply(std::vector<objectOrValue>& args, SymbolTable* symtab, ArgState& argState)
	{
		double result = 0;
		bool defined = false;
		for (objectOrValue arg : args)
		{
			auto valueHeld VALUEHELD(arg);
			if (!defined)
			{
				result = getNumericalValue(valueHeld);
				defined = true;
			}
			else
			{
				result *= getNumericalValue(valueHeld);
			}
		}
		return ast::value(result);
	}
	/// <summary>
	/// Divides the value of all args and returns the result
	/// </summary>
	/// <returns>Sum of all args</returns>
	objectOrValue Divide(std::vector<objectOrValue>& args, SymbolTable* symtab, ArgState& argState)
	{
		double result = 0;
		bool defined = false;
		for (objectOrValue arg : args)
		{
			auto valueHeld VALUEHELD(arg);
			if (!defined)
			{
				result = getNumericalValue(valueHeld);
				defined = true;
			}
			else
			{
				result /= getNumericalValue(valueHeld);
			}
		}
		return ast::value(result);
	}

	/// <summary>
	/// Modulo function
	/// </summary>
	/// <returns></returns>
	objectOrValue Mod(std::vector<objectOrValue>& args, SymbolTable* symtab, ArgState& argState)
	{
		int result = 0;
		bool defined = false;
		for (objectOrValue arg : args)
		{
			auto valueHeld VALUEHELD(arg);
			if (!defined)
			{
				result = static_cast<int>(getNumericalValue(valueHeld));
				defined = true;
			}
			else
			{
				result = result % static_cast<int>(getNumericalValue(valueHeld));
			}
		}
		return ast::value(result);
	}

	/// <summary>
	/// Whether or not arg 0 is equal to arg 1
	/// </summary>
	/// <param name="args"></param>
	/// <param name="symtab"></param>
	/// <param name="argState"></param>
	/// <returns></returns>
	objectOrValue Equal(std::vector<objectOrValue>& args, SymbolTable* symtab, ArgState& argState)
	{
		if (args.size() > 1)
		{
			auto val1 = VALUEHELD(args.at(0));
			auto val2 = VALUEHELD(args.at(1));
			if (std::holds_alternative<std::string>(val1) and std::holds_alternative<std::string>(val2))
				return val1 == val2;
			// Only really needed for stoi
			return getNumericalValue(VALUEHELD(args.at(0))) == getNumericalValue(VALUEHELD(args.at(1)));
		}
		return Zero;
	}

	/// <summary>
	/// Whether or not arg 0 is larger than arg 1
	/// </summary>
	/// <param name="args"></param>
	/// <param name="symtab"></param>
	/// <param name="argState"></param>
	/// <returns></returns>
	objectOrValue LargerThan(std::vector<objectOrValue>& args, SymbolTable* symtab, ArgState& argState)
	{
		if (args.size() > 1)
		{
			return getNumericalValue(VALUEHELD(args.at(0))) > getNumericalValue(VALUEHELD(args.at(1)));
		}
		return Zero;
	}
	
	// This is kind of terrible but I don't feel like copying the same code 10000 times
	SINGLE_ARG_FUNCTION(sin, Sine);
	SINGLE_ARG_FUNCTION(cos, Cosine);
	SINGLE_ARG_FUNCTION(tan, Tangent);
	SINGLE_ARG_FUNCTION(acos, ArcCosine);
	SINGLE_ARG_FUNCTION(asin, ArcSine);
	SINGLE_ARG_FUNCTION(atan, ArcTangent);
	DOUBLE_ARG_FUNCTION(atan2, ArcTangent2);
	DOUBLE_ARG_FUNCTION(pow, Power);
	SINGLE_ARG_FUNCTION(sqrt, SquareRoot);
	SINGLE_ARG_FUNCTION(floor, Floor);
	SINGLE_ARG_FUNCTION(ceil, Ceiling);
	SINGLE_ARG_FUNCTION(round, Round);
	SINGLE_ARG_FUNCTION(log, NaturalLogarithm);
}