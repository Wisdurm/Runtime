#pragma once
// This file contains all the functions for the StandardLibrary in Runtime
#include "StandardFiles.h"
#include "../symbol_table.h"
#include "../interpreter.h"
#include "../parser.h"
#include "../shared_libs.h"
// C++
#include <deque>
#include <iostream>
#include <fstream>
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>
#include <variant>
#include <experimental/memory>
#include <iomanip>
#include <sstream>
// Gnu
#include <readline/readline.h>
// C
#include <stdlib.h>
#include <ffi.h>

namespace rt
{
	/// <summary>
	/// Maps type names to their ffi type pointers
	/// </summary>
	static const std::unordered_map<std::string, CType> typeNames = {
		{"void", CType::Void}, // Only for return values
		{"uint8", CType::Uint8}, // u int8
		{"sint8", CType::Sint8}, // s int8
		{"int8", CType::Sint8}, // Default
		{"uint16", CType::Uint16}, // u int16
		{"sint16", CType::Sint16}, // s int16
		{"int16", CType::Sint16}, // Default
		{"uint32", CType::Uint32}, // u int32
		{"sint32", CType::Sint32}, // s int32
		{"int32", CType::Sint32}, // Default
		{"uint64", CType::Uint64}, // u int64
		{"sint64", CType::Sint64}, // s int64
		{"int64", CType::Sint64}, // Default
		{"float", CType::Float},
		{"double", CType::Double},
		{"uchar", CType::Uchar}, // u char
		{"schar", CType::Schar}, // s char
		// No default char since the standard does not specify whether or not
		// char is signed by default
		{"ushort", CType::Ushort}, // u short
		{"sshort", CType::Sshort}, // s short
		{"short", CType::Sshort}, // Default
		{"uint", CType::Uint}, // u int
		{"sint", CType::Sint}, // s int
		{"int", CType::Sint}, // Int is signed by default, so we have default :)
		{"ulong", CType::Ulong}, // u long
		{"slong", CType::Slong}, // s long
		{"long", CType::Slong}, // Default
		{"longdouble", CType::Longdouble},
		// Complex
		{"complex_float", CType::Complexfloat},
		{"complex_double", CType::Complexdouble},
		{"complex_longdouble", CType::Complexlongdouble},
		// Other
		{"cstring", CType::Cstring}
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
			auto valueHeld = evaluate(arg, symtab, argState);
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
	/// Identical to Object but evaluates
	/// </summary>
	/// <param name="args"></param>
	/// <returns></returns>
	objectOrValue Append(std::vector<objectOrValue>& args, SymbolTable* symtab, ArgState& argState)
	{
		if (args.size() > 0 and std::holds_alternative<std::shared_ptr<Object>>(args.at(0)))
		{
			std::shared_ptr<Object> init = std::get<std::shared_ptr<Object>>(args.at(0)); // Main object to initialize
			for (std::vector<objectOrValue>::iterator it = ++args.begin(); it != args.end(); ++it)
			{
				init.get()->addMember(evaluate(*it, symtab, argState, true));
			}
			return True;
		}
		return giveException("Incorrect arguments");
	}

	/// <summary>
	/// Identical to Append but uses soft evaluate
	/// </summary>
	/// <param name="args"></param>
	/// <returns></returns>
	objectOrValue Copy(std::vector<objectOrValue>& args, SymbolTable* symtab, ArgState& argState)
	{
		if (args.size() > 0 and std::holds_alternative<std::shared_ptr<Object>>(args.at(0)))
		{
			std::shared_ptr<Object> init = std::get<std::shared_ptr<Object>>(args.at(0)); // Main object to initialize
			for (std::vector<objectOrValue>::iterator it = ++args.begin(); it != args.end(); ++it)
			{
				init.get()->addMember(softEvaluate(*it, symtab, argState, true));
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
			auto key = evaluate(args.at(1), symtab, argState);
			assignee.get()->setMember(key, evaluate(args.at(2),symtab,argState,false));
			return True;
		}
		return giveException("Incorrect arguments");
	}

	/// <summary>
	/// Identical to Assign, but does not evaluate
	/// </summary>
	/// <param name="args">First arg is object/member to assign to, second one is the key of the member and the third one is the value to assign</param>
	/// <returns></returns>
	objectOrValue Update(std::vector<objectOrValue>& args, SymbolTable* symtab, ArgState& argState)
	{
		if (args.size() > 0 and std::holds_alternative<std::shared_ptr<Object>>(args.at(0)))
		{
			std::shared_ptr<Object> assignee = std::get<std::shared_ptr<Object>>(args.at(0)); // Object to assign value to
			auto key = evaluate(args.at(1),symtab, argState, true);
			assignee.get()->setMember(key, args.at(2));
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
			auto r = evaluate(args.at(0), symtab, argState);
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
			auto valueHeld = evaluate(arg, symtab, argState);
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
					loadSharedLibrary(fileName.c_str(), symtab); // Call function
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
			auto valueHeld = evaluate(args.at(0), symtab, argState);
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
			while (toBoolean(evaluate(args.at(0), symtab, argState, false)))
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
			auto valueHeld = evaluate(args.at(0), symtab, argState);
			return static_cast<double>(not toBoolean(valueHeld));
		}
		return giveException("Wrong amount of arguments");
	}
	
	/*
	 * Formats together all args into a string
	 * Added=v0.9.8
	 * Returns=A formatted string or an exception.
	 * Param0[True]Format=A string representing the structure.
	 * Params[True]Values=Values to be formatted into the string.
	 */
	objectOrValue Format(std::vector<objectOrValue>& args, SymbolTable* symtab, ArgState& argState)
	{
		std::string format;
		if (args.size() > 0)
		{
			auto val = evaluate(args.at(0), symtab, argState);
			if (std::holds_alternative<std::string>(val))
				format = std::get<std::string>(val);
			else
				return giveException("Format is of wrong type");
		}
		std::vector<std::variant<double, std::string>> values;
		for(std::vector<objectOrValue>::iterator it = args.begin()+1; it != args.end(); ++it )
		{
			values.push_back(evaluate(*it, symtab, argState));
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

	/*
	 * Returns all keys from the symbol table of the current scope.
	 * Added=v0.11.0
	 * Returns=An object with key names as it's members.
	 */
	objectOrValue GetKeys(std::vector<objectOrValue>& args, SymbolTable* symtab, ArgState& argState)
	{
		auto smt = std::make_shared<Object>("symbols");
		for (auto i : symtab->getKeys()) {
			std::variant<double, std::string> v = i; // ast::value :(
			smt->addMember(v);
		}
		return smt;
	}

	/*
	 * Desc=Returns how many members an object has.
	 * Added=v0.11.0
	 * Returns=Number of members or exception
	 * Param0[False]Object=An object to inspect.
	 */
	objectOrValue Size(std::vector<objectOrValue>& args, SymbolTable* symtab, ArgState& argState)
	{
		if (args.size() > 0)
		{
			if (std::holds_alternative<std::variant<double, std::string>>(args.at(0))) {
				return giveException("Object must be object");
			}
			auto obj = std::get<std::shared_ptr<Object>>(args.at(0));
			return static_cast<double>(obj->size());
		}
		return giveException("Wrong amount of arguments");
	}

	[[nodiscard]] Type makeType(objectOrValue& obv, SymbolTable* symtab, ArgState& argState)
	{
		// Trusting move elision with my life
		if (const auto obj = std::get_if<std::shared_ptr<Object>>(&obv)) {
			// Struct
			std::vector<Type> members;
			for (auto m : (*obj)->getMembers()) {
				members.push_back(std::move(makeType(m, symtab, argState)));
			}
			return Type(CType::Struct, false, members);
		} else {
			// Not struct
			auto rV = evaluate(obv, symtab, argState);
			if (const std::string* rType = std::get_if<std::string>(&rV)) {
				// Is pointer type
				if (rType->back() == '*') {
					return Type(typeNames.at(rType->substr(0, rType->size() - 1)), true);
				} else {
					return Type(typeNames.at(*rType), false);
				}
			} else [[unlikely]] {
				throw;
			}
		}
	}
	
	/*
	 * Desc=Creates a binding for a shared function, by specifying it's parameters and return value.
	 * Added=v0.11.0
	 * Returns=1 or exception
	 * Param0[True]Name=The name of the function.
	 * Param1[True?]Return=The return type, either a string or an object representing a struct.
	 * Params[True?]Name=The argument types in order, either strings or objects representing structs.
	 */
	objectOrValue Bind(std::vector<objectOrValue>& args, SymbolTable* symtab, ArgState& argState)
	{
		// TODO: If fails midway through, undefined behaviour // ??
		if (args.size() < 2) [[unlikely]]
			return giveException("Wrong amount of arguments");
		LibFunc* func = nullptr; // Function to bind
		 // Get function by name
		{
			auto v = evaluate(args.at(0), symtab, argState);
			if (const std::string* name = std::get_if<std::string>(&v)){
				if ((func = std::get_if<std::shared_ptr<LibFunc>>(&symtab->lookUpHard(*name))->get())) {}
				else { [[unlikely]]
					return giveException("Func name was not of a shared function");
				}
			} else [[unlikely]] {
				return giveException("Func name was of wrong type");
			}
		}
		// Reset function
		func->argTypes.clear();
		func->initialized = false;
		// Get return value
		func->retType.emplace(std::move(makeType(args.at(1), symtab, argState)));
		// Get parameters
		for (auto it = args.begin() + 2; it != args.end(); ++it) {
			func->argTypes.emplace_back(std::move(makeType(*it, symtab, argState)));
		}
		// Finished
		func->initialized = true;
		return True;
	}

	/*
	 * Desc=Runs a shell command.
	 * Added=v0.11.0
	 * Returns=1 or exception.
	 * Param0[True]Command=String to run in the shell.
	 */
	objectOrValue System(std::vector<objectOrValue>& args, SymbolTable* symtab, ArgState& argState)
	{
		static bool works = false;
		if (not works and system(NULL)) // Check whether shell exists
			works = true;
		if (not works)
			return giveException("Shell enviroment does not exist"); // Not sure if this is true, TODO...

		if (args.size() > 0)
		{
			auto valueHeld = evaluate(args.at(0), symtab, argState);
			if (const std::string* cmd = std::get_if<std::string>(&valueHeld)) {
				system(cmd->c_str());
				return True;
			}
			return giveException("Argument was of wrong type");
		}
		return giveException("Wrong amount of arguments");
	}

	/*
	 * Desc=Evaluates all arguments and returns the last one.
	 * Added=v0.11.0
	 * Returns=The value of the last object
	 * Params[True]Objects=A list of objects, which will be evaluated in order.
	 */
	objectOrValue Series(std::vector<objectOrValue>& args, SymbolTable* symtab, ArgState& argState)
	{
		std::vector<objectOrValue>::iterator it = args.begin();
		for (; it != args.end() - 1; ++it) {
			evaluate(*it, symtab, argState);
		}
		// Return last one
		return evaluate(args.back(), symtab, argState);
	}

	/*
	 * Desc=Evaluates all arguments until one returns a falsy value.
	 * Added=v0.12.0
	 * Returns=The value of the false object, or 1
	 * Params[True]Objects=A list of objects, which may be evaluated.
	 */
	objectOrValue And(std::vector<objectOrValue>& args, SymbolTable* symtab, ArgState& argState)
	{
		std::vector<objectOrValue>::iterator it = args.begin();
		for (; it != args.end(); ++it) {
			auto value = evaluate(*it, symtab, argState);
			if (not toBoolean(value)) {
				return value;
			}
		}
		// Return false
		return True;
	}
	
	/*
	 * Desc=Evaluates all arguments until one returns a truthy value.
	 * Added=v0.12.0
	 * Returns=The value of the true object, or 0
	 * Params[True]Objects=A list of objects, which may be evaluated.
	 */
	objectOrValue Or(std::vector<objectOrValue>& args, SymbolTable* symtab, ArgState& argState)
	{
		std::vector<objectOrValue>::iterator it = args.begin();
		for (; it != args.end(); ++it) {
			auto value = evaluate(*it, symtab, argState);
			if (toBoolean(value)) {
				return value;
			}
		}
		// Return false
		return False;
	}

	/*
	 * Desc=Returns the name of an object.
	 * Added=v0.12.0
	 * Returns=Name of the object
	 * Param0[False]Object=An object to inspect.
	 */
	objectOrValue Name(std::vector<objectOrValue>& args, SymbolTable* symtab, ArgState& argState)
	{
		if (args.size() > 0)
		{
			if (std::holds_alternative<std::variant<double, std::string>>(args.at(0))) {
				return giveException("Object must be object");
			}
			auto obj = std::get<std::shared_ptr<Object>>(args.at(0));
			return obj->getName();
		}
		return giveException("Wrong amount of arguments");
	}
}
