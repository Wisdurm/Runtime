#pragma once
#include "StandardFiles.h"
#include "../interpreter.h"
#include "../parser.h"
// C++
#include <cmath>
#include <vector>
#include <variant>

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
	
	/// <summary>
	/// Sine of first value
	/// </summary>
	objectOrValue Sine(std::vector<objectOrValue>& args, SymbolTable* symtab, ArgState& argState)
	{
		if (args.size() > 0)
			return sin(getNumericalValue(VALUEHELD(args.at(0))));
		return Zero;
	}

	/// <summary>
	/// Cosine of first value
	/// </summary>
	objectOrValue Cosine(std::vector<objectOrValue>& args, SymbolTable* symtab, ArgState& argState)
	{
		if (args.size() > 0)
			return cos(getNumericalValue(VALUEHELD(args.at(0))));
		return Zero;
	}
}