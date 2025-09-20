#pragma once
#include "../interpreter.h"
#include "../parser.h"
// C++
#include <vector>
#include <variant>

namespace rt
{
	/// <summary>
	/// Returns the numerical value of ast::value.valueHeld
	/// </summary>
	/// <returns></returns>
	static double getNumericalValue(std::variant<long, double, std::string> val)
	{
		// Convert type to number
		if (std::holds_alternative<std::string>(val))
		{
			std::string str = std::get<std::string>(val);
			if (str.find('.') != std::string::npos)
				return std::stod(str);
			else
				return std::stoi(str);
		}
		else if (std::holds_alternative<long>(val))
			return std::get<long>(val);
		else
			return std::get<double>(val);
	}
	/// <summary>
	/// Adds the value of all args together and retuns the sum
	/// </summary>
	/// <returns>Sum of all args</returns>
	objectOrValue Add(std::vector<objectOrValue>& args, SymbolTable* symtab, ArgState& argState)
	{
		double sum = 0;
		for (objectOrValue arg : args)
		{
			auto valueHeld(std::holds_alternative<std::shared_ptr<Object>>(arg) ? // If object
				evaluate(std::get<std::shared_ptr<Object>>(arg), symtab, argState, true).valueHeld : // Get value of object
				std::get<ast::value>(arg).valueHeld // If value, just use value
			);
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
			auto valueHeld(std::holds_alternative<std::shared_ptr<Object>>(arg) ? // If object
				evaluate(std::get<std::shared_ptr<Object>>(arg), symtab, argState, true).valueHeld : // Get value of object
				std::get<ast::value>(arg).valueHeld // If value, just use value
			);
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
	/// Divides the value of all args and retuns the result
	/// </summary>
	/// <returns>Sum of all args</returns>
	objectOrValue Divide(std::vector<objectOrValue>& args, SymbolTable* symtab, ArgState& argState)
	{
		double result = 0;
		bool defined = false;
		for (objectOrValue arg : args)
		{
			auto valueHeld(std::holds_alternative<std::shared_ptr<Object>>(arg) ? // If object
				evaluate(std::get<std::shared_ptr<Object>>(arg), symtab, argState, true).valueHeld : // Get value of object
				std::get<ast::value>(arg).valueHeld // If value, just use value
			);
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
			auto valueHeld(std::holds_alternative<std::shared_ptr<Object>>(arg) ? // If object
				evaluate(std::get<std::shared_ptr<Object>>(arg), symtab, argState, true).valueHeld : // Get value of object
				std::get<ast::value>(arg).valueHeld // If value, just use value
			);
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
}