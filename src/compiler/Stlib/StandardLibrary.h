#pragma once
// This file contains all the functions for the StandardLibrary in Runtime
#include "../interpreter.h"
// C++
#include <iostream>
#include <vector>
#include <variant>

namespace rt
{
	static ast::value Zero = ast::value(0);
	/// <summary>
	/// Prints a value to the standard output
	/// </summary>
	/// <param name="args">Value(s) to print</param>
	objectOrValue Print(std::vector<objectOrValue>& args, SymbolTable* symtab)
	{
		for (objectOrValue arg : args)
		{
			auto valueHeld(std::holds_alternative<std::shared_ptr<Object>>(arg) ? // If object
				evaluate(std::get<std::shared_ptr<Object>>(arg), symtab).valueHeld : // Get value of object
				std::get<ast::value>(arg).valueHeld // If value, just use value
			);

			std::string output;
			// Convert type to string
			if (std::holds_alternative<std::string>(valueHeld))
				output = std::get<std::string>(valueHeld);
			else if (std::holds_alternative<long>(valueHeld))
				output = std::to_string(std::get<long>(valueHeld));
			else
				output = std::to_string(std::get<double>(valueHeld));
			
			// Print
			if (isCapture())
				captureString(output);
			// TODO: Add else clause if not debugging
			std::cout << output;
		}
		std::cout << std::endl;
		return Zero;
	}
	/// <summary>
	/// Initializes object with specified arguments
	/// </summary>
	/// <param name="args"></param>
	/// <returns></returns>
	objectOrValue ObjectF(std::vector<objectOrValue>& args, SymbolTable* symtab)
	{
		if (args.size() > 0 and std::holds_alternative<std::shared_ptr<Object>>(args.at(0)))
		{
			std::shared_ptr<Object> init = std::get<std::shared_ptr<Object>>(args.at(0)); // Main object to initialize
			for (std::vector<objectOrValue>::iterator it = ++args.begin(); it != args.end(); ++it)
			{
				init.get()->addMember(*it);
			}
			return init;
		}
		return Zero;
	}
}