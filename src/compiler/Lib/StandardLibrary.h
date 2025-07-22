#pragma once
// This file contains all the functions for the StandardLibrary in Runtime
#include "../interpreter.h"
// C++
#include <iostream>
#include <vector>
#include <variant>

namespace rt
{
	/// <summary>
	/// Prints a value to the standard output
	/// </summary>
	/// <param name="args">Value(s) to print</param>
	Object Print(std::vector<Object>& args)
	{
		for (Object& arg : args)
		{
			auto v = std::get<ast::value>(arg.getMember(0)).valueHeld; // Get value held by first member of object
			std::string output;
			// Convert type to string
			if (std::holds_alternative<std::string>(v))
				output = std::get<std::string>(v);
			else if (std::holds_alternative<long>(v))
				output = std::to_string(std::get<long>(v));
			else
				output = std::to_string(std::get<double>(v));
			
			// Print
			if (isCapture())
				captureString(output);
			std::cout << output;
		}
		std::cout << std::endl;
		return Object();
	}
}