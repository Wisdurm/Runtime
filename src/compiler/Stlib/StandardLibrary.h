#pragma once
// This file contains all the functions for the StandardLibrary in Runtime
#include "StandardFiles.h"
#include "../interpreter.h"
#include "../parser.h"
// C++
#include <iostream>
#include <fstream>
#include <unordered_map>
#include <vector>
#include <variant>
#include <experimental/memory>
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
	/// Maps type names to their ffi type pointers
	/// </summary>
	static const std::unordered_map<std::string, ffi_type*> typeNames = {
		{"void", &ffi_type_void}, // Only for return values
		{"uint8", &ffi_type_uint8}, // u int8
		{"sint8", &ffi_type_sint8}, // s int8
		{"int8", &ffi_type_sint8}, // Default
		{"uint16", &ffi_type_uint16}, // u int16
		{"sint16", &ffi_type_sint16}, // s int16
		{"int16", &ffi_type_sint16}, // Default
		{"uint32", &ffi_type_uint32}, // u int32
		{"sint32", &ffi_type_sint32}, // s int32
		{"int32", &ffi_type_sint32}, // Default
		{"uint64", &ffi_type_uint64}, // u int64
		{"sint64", &ffi_type_sint64}, // s int64
		{"int64", &ffi_type_sint64}, // Default
		{"float", &ffi_type_float},
		{"double", &ffi_type_double},
		{"uchar", &ffi_type_uchar}, // u char
		{"schar", &ffi_type_schar}, // s char
		// No default char since the standard does not specify whether or not
		// char is signed by default :/
		{"ushort", &ffi_type_ushort}, // u short
		{"sshort", &ffi_type_sshort}, // s short
		{"short", &ffi_type_sshort}, // Default
		{"uint", &ffi_type_uint}, // u int
		{"sint", &ffi_type_sint}, // s int
		{"int", &ffi_type_sint}, // Int is signed by default, so we have default :)
		{"ulong", &ffi_type_ulong}, // u long
		{"slong", &ffi_type_ulong}, // s long
		{"long", &ffi_type_ulong}, // Default
		{"longdouble", &ffi_type_longdouble},
		{"pointer", &ffi_type_pointer}, // Generic pointer
		{"cstring", &ffi_type_cstring}, // Char pointer, TODO: interpreter should make this a string
		{"complex_float", &ffi_type_complex_float},
		{"complex_double", &ffi_type_complex_double},
		{"complex_longdouble", &ffi_type_complex_longdouble},
	};
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
		return giveException("Incorrect arguments");
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
		return giveException("Incorrect arguments");
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
	/// <param name="args">Args are either the names of .rnt runtime files, or shared libraries (.so) following the C calling conventions</param>
	/// <param name="symtab"></param>
	/// <returns></returns>
	objectOrValue Include(std::vector<objectOrValue>& args, SymbolTable* symtab, ArgState& argState)
	{
		for (objectOrValue arg : args)
		{
			auto valueHeld VALUEHELD(arg);
			if (std::holds_alternative<std::string>(valueHeld)) // If string
			{
				std::string fileName = std::get<std::string>(valueHeld);
				std::ifstream file;
				// Runtime library
				if (fileName.ends_with(".rnt")) {
					// Open file
					file.open(fileName, std::fstream::in);
					if (file.fail())
					{
						return giveException("Unable to open file");
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
				else if (fileName.ends_with(".so") or fileName.ends_with(".dll")) {
					loadSharedLibrary(fileName.c_str()); // Call function
					return True;
				}
				else {
					return giveException("File is not a Runtime file or a shared library");
				}
			}
			else {
				return giveException("Filepath is of wrong type");
			}
		}
		return giveException("Wrong amount of arguments");
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
		return giveException("Wrong amount of arguments");
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
		return giveException("Wrong amount of arguments");
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
		return giveException("Wrong amount of arguments");
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
				return giveException("Format is of wrong type");
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

	// On hold while I figure out what I want to do with my life
	/* /// <summary> */
	/* ///	Returns all keys from the symbol table of the current scope */
	/* /// </summary> */
	/* /// <param name="args"></param> */
	/* /// <param name="symtab"></param> */
	/* /// <param name="argState"></param> */
	/* /// <returns></returns> */
	/* objectOrValue GetKeys(std::vector<objectOrValue>& args, SymbolTable* symtab, ArgState& argState) */
	/* { */
	/* 	auto smt = std::make_shared<Object>("symbols"); */
	/* 	for (auto i : symtab->getKeys()) { */
	/* 		std::variant<double, std::string> v = i; // ast::value :( */
	/* 		smt->addMember(v); */
	/* 	} */
	/* 	return smt; */
	/* } */

	/// <summary>
	///	Returns the size of an object; that is, how many members it has
	/// </summary>
	/// <param name="args"></param>
	/// <param name="symtab"></param>
	/// <param name="argState"></param>
	/// <returns></returns>
	objectOrValue Size(std::vector<objectOrValue>& args, SymbolTable* symtab, ArgState& argState)
	{
		if (args.size() > 0)
		{
			if (std::holds_alternative<std::variant<double, std::string>>(args.at(0))) {
				return giveException("Object must be object");
			}
			auto obj = std::get<std::shared_ptr<Object>>(args.at(0));
			return static_cast<double>(obj->getMembers().size());
		}
		return giveException("Wrong amount of arguments");
	}

	/// <summary>
	///	Creates a binding for a shared function, by specifying it's parameters and return value.
	///	Arg0 is the name of the function, arg1 is the return type and the rest of the args are 
	///	parameter types.
	/// </summary>
	/// <param name="args"></param>
	/// <param name="symtab"></param>
	/// <param name="argState"></param>
	/// <returns></returns>
	objectOrValue Bind(std::vector<objectOrValue>& args, SymbolTable* symtab, ArgState& argState)
	{
		// TODO: If fails midway through, undefined behaviour
		if (args.size() < 2)
			return giveException("Wrong amount of arguments");
		LibFunc* func = nullptr; // Function to bind
		 // Get function by name
		{
			auto v = VALUEHELD(args.at(0));
			if (const std::string* name = std::get_if<std::string>(&v)){
				if ((func = std::get_if<LibFunc>(&globalSymtab.lookUpHard(*name)))) {}
				else {
					return giveException("Func name was not of a shared function");
				}
			}
			else
				return giveException("Func name was of wrong type");
		}
		// Reset function
		func->argTypes.clear();
		func->initialized = false;
		func->retType = std::experimental::make_observer<ffi_type>(nullptr); // Unbelievable
		// Get return value
		{
			ffi_type* ret;
			auto rV = VALUEHELD(args.at(1));
			if (const std::string* rType = std::get_if<std::string>(&rV)){
				ret = typeNames.at(*rType);
			}
			else
				return giveException("Return name was of wrong type");
			func->retType = std::experimental::make_observer<ffi_type>(ret);
		}
		// Get parameters
		func->argTypes.reserve(args.size() - 2);; // Prepare for args
		for (auto it = args.begin() + 2; it != args.end(); ++it) {
			auto rP = VALUEHELD(*it);
			if (const std::string* pType = std::get_if<std::string>(&rP)){ 
				func->argTypes.push_back(std::experimental::make_observer<ffi_type>(typeNames.at(*pType)));
				if (*pType == "void") { // TODO: Faster comparison
					return giveException("Shared function cannot have parameters of type void");
				}
			} else { // Struct
				ffi_type* e[] = {&ffi_type_sint, &ffi_type_float, NULL};
				func->argTypes.push_back(std::make_shared<ffi_type>(
					0, // size (init 0)
					0, // align (init 0)
					FFI_TYPE_STRUCT,// type
					e// elements
							));
			}
		}
		// Finished
		func->initialized = true;
		return True;
	}
}
