#pragma once
// This file contains all the math functions in Runtime
#include "StandardFiles.h"
#include "../interpreter.h"
// C++
#include <cmath>
#include <stdexcept>
#include <vector>
#include <variant>
#include <cstdlib>

// Automatically make single argument functions from C++ functions
#define SINGLE_ARG_FUNCTION(func, name) objectOrValue name(std::vector<objectOrValue>& args, SymbolTable* symtab, ArgState& argState) \
	{ if (args.size() > 0) \
		return func(getNumericalValue(getValue(args.at(0), symtab, argState)));	\
		else return False; }

// Same thing but two args
#define DOUBLE_ARG_FUNCTION(func, name) objectOrValue name(std::vector<objectOrValue>& args, SymbolTable* symtab, ArgState& argState) \
	{ if (args.size() > 1) \
		return func(getNumericalValue(getValue(args.at(0), symtab, argState)), getNumericalValue(getValue(args.at(1), symtab, argState)));	\
		else return False; }

namespace rt
{ /// <summary>
	/// Adds the value of all args together and retuns the sum
	/// </summary>
	/// <returns>Sum of all args</returns>
	objectOrValue Add(std::vector<objectOrValue>& args, SymbolTable* symtab, ArgState& argState)
	{
		double sum = 0;
		for (objectOrValue arg : args)
		{
			auto valueHeld = getValue(arg, symtab, argState);
			sum += getNumericalValue(valueHeld);
		}
		return std::variant<double, std::string>(sum);
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
			auto valueHeld = getValue(arg, symtab, argState);
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
		return std::variant<double, std::string>(result);
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
			auto valueHeld = getValue(arg, symtab, argState);
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
		return std::variant<double, std::string>(result);
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
			auto valueHeld = getValue(arg, symtab, argState);
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
		return std::variant<double, std::string>(result);
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
			auto valueHeld = getValue(arg, symtab, argState);
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
		return static_cast<double>(result);
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
			// Check if literally the same object, as in same memory address
			if (std::holds_alternative<std::shared_ptr<Object>>(args.at(0)) and std::holds_alternative<std::shared_ptr<Object>>(args.at(1))
				and std::get<std::shared_ptr<Object>>(args.at(0)).get() == std::get<std::shared_ptr<Object>>(args.at(1)).get()) 
				return 1.0;
			auto val1 = getValue(args.at(0), symtab, argState);
			auto val2 = getValue(args.at(1), symtab, argState);
			if (std::holds_alternative<std::string>(val1) and std::holds_alternative<std::string>(val2))
				return static_cast<double>(val1 == val2);
			// Only really needed for stoi
			try {
				return static_cast<double>(getNumericalValue(getValue(args.at(0), symtab, argState))
						== getNumericalValue(getValue(args.at(1), symtab, argState)));
			}catch(std::invalid_argument) {
				return giveException("Uncomparable arguments");
			}
		}
		return giveException("Wrong amount of arguments");
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
			return static_cast<double>(getNumericalValue(getValue(args.at(0), symtab, argState))
					> getNumericalValue(getValue(args.at(1), symtab, argState)));
		}
		return giveException("Wrong amount of arguments");
	}
	
	/// <summary>
	/// Whether or not arg 0 is smaller than arg 1
	/// </summary>
	/// <param name="args"></param>
	/// <param name="symtab"></param>
	/// <param name="argState"></param>
	/// <returns></returns>
	objectOrValue SmallerThan(std::vector<objectOrValue>& args, SymbolTable* symtab, ArgState& argState)
	{
		if (args.size() > 1)
		{
			return static_cast<double>(getNumericalValue(getValue(args.at(0), symtab, argState))
					< getNumericalValue(getValue(args.at(1), symtab, argState)));
		}
		return giveException("Wrong amount of arguments");
	}

	/// <summary>
	/// Returns a random number between 0 and the integerlimit
	/// </summary>
	/// <returns></returns>
	objectOrValue RandomInteger(std::vector<objectOrValue>& args, SymbolTable* symtab, ArgState& argState)
	{
		srand(time(NULL)); // Need only be run once, but not sure where else to put this :shrug:
		return static_cast<double>(rand());
	}

	/// <summary>
	/// Returns a random decimal number between 0 and 1
	/// </summary>
	/// <returns></returns>
	objectOrValue RandomDecimal(std::vector<objectOrValue>& args, SymbolTable* symtab, ArgState& argState)
	{
		srand(time(NULL)); // Need only be run once, but not sure where else to put this :shrug:
		return static_cast<double>(rand()) / RAND_MAX;
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
