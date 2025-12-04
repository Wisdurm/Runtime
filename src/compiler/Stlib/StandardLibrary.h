#pragma once
// This file contains all the functions for the StandardLibrary in Runtime
#include "StandardFiles.h"
#include "../interpreter.h"
#include "../parser.h"
// C++
#include <iostream>
#include <fstream>
#include <vector>
#include <variant>
// Gnu
#include <readline/readline.h>
// C
#include <stdlib.h>

#include <cctype>      // std::tolower
#include <algorithm>   // std::equal
#include <string_view> // std::string_view

#include <iomanip>
#include <sstream>

namespace rt
{
	/// <summary>
	/// Returns the first argument
	/// </summary>
	/// <param name="args"></param>
	/// <param name="symtab"></param>
	/// <param name="argState"></param>
	/// <returns></returns>
	objectOrValue Return(std::vector<objectOrValue>& args, SymbolTable* symtab, ArgState& argState)
	{
		return args.at(0);
	}
	/// <summary>
	/// Prints a value to the standard output
	/// </summary>
	/// <param name="args">Value(s) to print</param>
	objectOrValue Print(std::vector<objectOrValue>& args, SymbolTable* symtab, ArgState& argState)
	{
		for (objectOrValue arg : args)
		{
			auto valueHeld VALUEHELD(arg);

			std::string output;
			// Convert type to string
			if (std::holds_alternative<std::string>(valueHeld))
				output = std::get<std::string>(valueHeld);
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
		return True;
	}

	/// Retrives a line from std::cin
	objectOrValue Input(std::vector<objectOrValue>& args, SymbolTable* symtab, ArgState& argState)
	{
		char* tmp = readline(" ");
		std::string r(tmp);
		free(tmp);
		return r;
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
			return True;
		}
		return False;
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
			std::variant<double, std::string> key = evaluate(args.at(1),symtab, argState, true);
			assignee.get()->setMember(key, evaluate(args.at(2),symtab,argState,false)  );
			return True;
		}
		return False;
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
			if (std::holds_alternative<double>(r))
				exit(std::get<double>(r));
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
			auto valueHeld VALUEHELD(arg);
			if (std::holds_alternative<std::string>(valueHeld)) // If string, try load file
			{
				std::string fileName = std::get<std::string>(valueHeld);
				std::ifstream file;
				// Open file
				file.open(fileName, std::fstream::in);
				if (file.fail())
				{
					std::cout << "Unable to open file " << fileName << std::endl;
					return False;
				}
				file.seekg(0, std::ios::end);
				size_t size = file.tellg();
				std::string fileText(size, ' ');
				file.seekg(0);
				file.read(&fileText[0], size);
				file.close();

				rt::include(rt::parse((rt::tokenize(fileText.c_str(), fileName.c_str())), true), symtab, argState);
				return True;
			}
		}
		return False;
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
			auto valueHeld VALUEHELD(args.at(0));
			// Evaluate cond
			bool cond = toBoolean(valueHeld);
			
			// Actual if statement
			if (cond)
			{
				return evaluate(args.at(1), symtab, argState, false);
			}
			else if (args.size() > 2)
			{
				return evaluate(args.at(2), symtab, argState, false);
			}
			return True;
		}
		return False;
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
			while (toBoolean(VALUEHELD_E(args.at(0))))// If value, just use value
			{ 
				std::vector<objectOrValue>::iterator it = args.begin() + 1;
				while (it != args.end())
				{
					evaluate(*it, symtab, argState, false);
					it++;
				}
			}
			return True;
		}
		return False;
	}

	/// <summary>
	///	Inverts a boolean contained within
	/// </summary>
	/// <param name="args"></param>
	/// <param name="symtab"></param>
	/// <param name="argState"></param>
	/// <returns></returns>
	objectOrValue Not(std::vector<objectOrValue>& args, SymbolTable* symtab, ArgState& argState)
	{
		if (args.size() > 0)
		{
			auto valueHeld = VALUEHELD(args.at(0));
			return static_cast<double>(not toBoolean(valueHeld));
		}
		return False; // Not really detectable :)
	}
	
	/// <summary>
	///	Formats together all args into a string
	/// </summary>
	/// <param name="args"></param>
	/// <param name="symtab"></param>
	/// <param name="argState"></param>
	/// <returns></returns>
	objectOrValue Format(std::vector<objectOrValue>& args, SymbolTable* symtab, ArgState& argState)
	{
		std::string format;
		if (args.size() > 0)
		{
			auto val = VALUEHELD(args.at(0));
			if (std::holds_alternative<std::string>(val))
				format = std::get<std::string>(val);
			else
				return False;
		}
		std::vector<std::variant<double, std::string>> values;
		for(std::vector<objectOrValue>::iterator it = args.begin()+1; it != args.end(); ++it )
		{
			values.push_back(VALUEHELD(*it));
		}
		// snprintf and std::format require args... soooo have to do this myself
		std::string result = "";
		int vali = 0;
		for (int i = 0; i < format.size(); i++)
		{
			if (format.at(i) == '$')
			{
				int digits = 6;
				if (i+1 < format.size() and isdigit(format.at(i+1))) // If num, specify double decimal digits
				{
					digits = format.at(i+1) - '0';
					i++;
				}
				if (std::holds_alternative<std::string>(values.at(vali)))
					result += std::get<std::string>(values.at(vali));
				else
				{
					// I love C++ :)
					std::stringstream stream;
					stream << std::fixed << std::setprecision(digits) << std::get<double>(values.at(vali));
					result += stream.str();
				}
				vali++;
			}
			else if (format.at(i) == '\\')
			{
				i++;
				switch (format.at(i))
				{
				case '\\': result += '\\'; break;
				case 'n': result += '\n'; break;
				case '$': result += '$'; break;
				case 't': result += '\t'; break;
				case 'r': result += '\r'; break;
				case '"': result += '\"'; break;
				default:
					break;
				}
			}
			else
				result += format.at(i);
		}

		return result;
	}
}
