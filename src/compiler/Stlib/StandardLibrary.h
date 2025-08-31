#pragma once
// This file contains all the functions for the StandardLibrary in Runtime
#include "../interpreter.h"
#include "../parser.h"
// C++
#include <iostream>
#include <fstream>
#include <vector>
#include <variant>

namespace rt
{
	static const ast::value Zero = ast::value(0);
	/// <summary>
	/// Prints a value to the standard output
	/// </summary>
	/// <param name="args">Value(s) to print</param>
	objectOrValue Print(std::vector<objectOrValue>& args, SymbolTable* symtab, ArgState& argState)
	{
		for (objectOrValue arg : args)
		{
			auto valueHeld(std::holds_alternative<std::shared_ptr<Object>>(arg) ? // If object
				evaluate(std::get<std::shared_ptr<Object>>(arg), symtab, argState).valueHeld : // Get value of object
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
			else // Remove else clause if debugging output
				std::cout << output << std::endl;
		}
		if (not isCapture)
			std::cout << std::endl;
		return Zero;
	}

	/// <summary>
	/// Initializes object with specified arguments
	/// </summary>
	/// <param name="args"></param>
	/// <returns></returns>
	objectOrValue ObjectF(std::vector<objectOrValue>& args, SymbolTable* symtab, ArgState& argState)
	{
		if (args.size() > 0 and std::holds_alternative<std::shared_ptr<Object>>(args.at(0)))
		{
			std::shared_ptr<Object> init = std::get<std::shared_ptr<Object>>(args.at(0)); // Main object to initialize
			for (std::vector<objectOrValue>::iterator it = ++args.begin(); it != args.end(); ++it)
			{
				init.get()->addMember(*it);
			}
			return Zero;
		}
		return Zero;
	}

	/// <summary>
	/// Assigns a value to an object/member
	/// </summary>
	/// <param name="args">First arg is object/member to assign to, second one is the key of the member and the third one is the value to assign</param>
	/// <returns></returns>
	objectOrValue Assign(std::vector<objectOrValue>& args, SymbolTable* symtab, ArgState& argState)
	{
		if (args.size() > 0 and std::holds_alternative<std::shared_ptr<Object>>(args.at(0)))
		{
			std::shared_ptr<Object> assignee = std::get<std::shared_ptr<Object>>(args.at(0)); // Object to assign value to
			if (std::holds_alternative<ast::value>(args.at(1)))
			{
				ast::value key = std::get<ast::value>(args.at(1));
				assignee.get()->setMember(key, args.at(2));
			}
		}
		return Zero;
	}

	/// <summary>
	/// Exits
	/// </summary>
	/// <param name="args">First arg is exit code</param>
	/// <param name="symtab"></param>
	/// <returns></returns>
	objectOrValue Exit(std::vector<objectOrValue>& args, SymbolTable* symtab, ArgState& argState)
	{
		if (args.size() > 0)
		{
			auto r = rt::evaluate(args.at(0), symtab, argState);
			if (r.type == ast::valueType::INT)
				exit(std::get<long>(r.valueHeld));
			else if (r.type == ast::valueType::DEC)
				exit(std::get<double>(r.valueHeld));
		}
		exit(0);
	}

	/// <summary>
	/// Includes a file in the current runtime
	/// </summary>
	/// <param name="args">Args are either the names of .rnt runtime files, or builtin libraries that end with .h</param>
	/// <param name="symtab"></param>
	/// <returns></returns>
	objectOrValue Include(std::vector<objectOrValue>& args, SymbolTable* symtab, ArgState& argState)
	{
		for (objectOrValue arg : args)
		{
			auto valueHeld(std::holds_alternative<std::shared_ptr<Object>>(arg) ? // If object
				evaluate(std::get<std::shared_ptr<Object>>(arg), symtab, argState).valueHeld : // Get value of object
				std::get<ast::value>(arg).valueHeld // If value, just use value
			);
			if (std::holds_alternative<std::string>(valueHeld)) // If string, try load file
			{
				std::string fileName = std::get<std::string>(valueHeld);
				std::ifstream file;
				// Open file
				file.open(fileName, std::fstream::in);
				if (file.fail())
				{
					std::cout << "Unable to open file " << fileName;
					return Zero;
				}
				file.seekg(0, std::ios::end);
				size_t size = file.tellg();
				std::string fileText(size, ' ');
				file.seekg(0);
				file.read(&fileText[0], size);
				file.close();

				rt::include(rt::parse((rt::tokenize(fileText.c_str())), true), symtab, argState);
			}
		}
		return Zero;
	}
}