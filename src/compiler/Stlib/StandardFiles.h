#pragma once
// This file defines functions and macros which are useful for the development of the StandardLibraries

namespace rt
{
	// Retrieves the value held by an object, or value. Uses ternary operator to do everything in a single line
#define VALUEHELD(x) (std::holds_alternative<std::shared_ptr<Object>>(x) ? /* If object */ \
evaluate(std::get<std::shared_ptr<Object>>(x), symtab, argState, true) : /* Get value of object */ \
	std::get<std::variant<double, std::string>>(x) /* If value, just use value */ \
	)
	// Variant of VALUEHELD which does not write evaluated values to memory
#define VALUEHELD_E(x) (std::holds_alternative<std::shared_ptr<Object>>(x) ? /* If object */ \
evaluate(std::get<std::shared_ptr<Object>>(x), symtab, argState, false) : /* Get value of object */ \
	std::get<std::variant<double, std::string>>(x) /* If value, just use value */ \
	)

	// Stack overflow
	bool ichar_equals(char a, char b)
	{
		return std::tolower(static_cast<unsigned char>(a)) ==
			std::tolower(static_cast<unsigned char>(b));
	}
	bool iequals(std::string_view lhs, std::string_view rhs)
	{
		return std::ranges::equal(lhs, rhs, ichar_equals);
	}
	// Evaluates a valueHeld as a bool
	bool toBoolean(std::variant<double, std::string> val)
	{
		if (std::holds_alternative<std::string>(val))
		{
			std::string str = std::get<std::string>(val);
			if (iequals(str, "true"))
				return true;
			else if (iequals(str, "false"))
				return false;
			else
			{
				for (char c : str)
				{
					if (not isalnum(c) or c == '.') // If string is not "true", "false" or a number, then it can't be evaluated as a boolean
						goto BAIL;
				}
				return std::stod(str) >= 1;
			}
		}
		else
		{
			return std::get<double>(val) >= 1;
		}
	BAIL:
		return 0;
	}
	/// <summary>
	/// Returns the numerical value of std::variant<double, std::string>.valueHeld
	/// </summary>
	/// <returns></returns>
	double getNumericalValue(std::variant<double, std::string> val)
	{
		// Convert type to number
		if (std::holds_alternative<std::string>(val))
		{
			return std::stod(std::get<std::string>(val));
		}
		else
			return std::get<double>(val);
	}
}
