#pragma once
// This file contains all the functions for the StandardLibrary in Runtime
#include "../interpreter.h"
#include "../parser.h"
// C++
#include <iostream>
#include <fstream>
#include <vector>
#include <variant>

#include <cctype>      // std::tolower
#include <algorithm>   // std::equal
#include <string_view> // std::string_view

namespace rt
{
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
	static bool toBoolean(std::variant<long, double, std::string> val)
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
				if (str.find('.') != std::string::npos)
				{
					return std::stod(str) >= 1;
				}
				else
				{
					return std::stoi(str) >= 1;
				}
			}
		}
		else if (std::holds_alternative<long>(val))
		{
			return std::get<long>(val) >= 1;
		}
		else
		{
			return std::get<double>(val) >= 1;
		}
	BAIL:
		return 0;
	}

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
				evaluate(std::get<std::shared_ptr<Object>>(arg), symtab, argState, true).valueHeld : // Get value of object
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
				std::cout << output;
		}
		if (not isCapture())
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
			ast::value key = evaluate(args.at(1),symtab, argState, true);
			assignee.get()->setMember(key, args.at(2));
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
			auto r = rt::evaluate(args.at(0), symtab, argState, true);
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
				evaluate(std::get<std::shared_ptr<Object>>(arg), symtab, argState, true).valueHeld : // Get value of object
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

	/// <summary>
	/// If statement
	/// </summary>
	/// <param name="args">If arg 0 is not zero, it then call arg 1. If arg 0 is zero, then call arg 2</param>
	/// <param name="symtab"></param>
	/// <param name="argState"></param>
	/// <returns></returns>
	objectOrValue If(std::vector<objectOrValue>& args, SymbolTable* symtab, ArgState& argState)
	{
		if (args.size() > 1)
		{
			auto valueHeld(std::holds_alternative<std::shared_ptr<Object>>(args.at(0)) ? // If object
				evaluate(std::get<std::shared_ptr<Object>>(args.at(0)), symtab, argState, true).valueHeld : // Get value of object
				std::get<ast::value>(args.at(0)).valueHeld // If value, just use value
			);
			// Evaluate cond
			bool cond = toBoolean(valueHeld);
			
			// Actual if statement
			if (cond)
			{
				return callObject(args.at(1), symtab, argState, {});
			}
			else if (args.size() > 2)
			{
				return callObject(args.at(2), symtab, argState, {});
			}
		}
		return Zero;
	}

	/// <summary>
	///	Evaluates a series of arguments as long as the first one is correct
	/// </summary>
	/// <param name="args"></param>
	/// <param name="symtab"></param>
	/// <param name="argState"></param>
	/// <returns></returns>
	objectOrValue While(std::vector<objectOrValue>& args, SymbolTable* symtab, ArgState& argState)
	{
		if (args.size() > 1)
		{
			while (toBoolean(
				(std::holds_alternative<std::shared_ptr<Object>>(args.at(0)) ? // If object
				evaluate(std::get<std::shared_ptr<Object>>(args.at(0)), symtab, argState, true).valueHeld : // Get value of object
				std::get<ast::value>(args.at(0)).valueHeld)))// If value, just use value
			{ 
				std::vector<objectOrValue>::iterator it = args.begin() + 1;
				while (it != args.end())
				{
					evaluate(*it, symtab, argState, false);
					it++;
				}
			}
		}
		return Zero;
	}
}