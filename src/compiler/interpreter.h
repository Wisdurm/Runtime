#pragma once
// Runtime
#include "object.h"
#include "shared_libs.h"
#include "ast.h"
// ??
#include <tsl/ordered_map.h>
// C++
#include <unordered_map> // Do testing later on to figure out if a normal map would be better
#include <string>
#include <memory>
#include <variant>
#include <functional>
#include <vector>
#include <algorithm>
#include <any>

/// <summary>
/// Built-in Runtime function
/// </summary>
using BuiltIn = std::function<objectOrValue(std::vector<objectOrValue>&, rt::SymbolTable*, rt::ArgState&)>;
/// <summary>
/// Type which symbol table points to object, a function object referencing a built in function or a function from a shared library.
/// </summary>
using Symbol = std::variant<std::shared_ptr<rt::Object>, BuiltIn, rt::LibFunc>;

namespace rt
{
	// TODO: Inline is scary but I must confront it someday

	/// <summary>
	/// Returns the numerical value of a value
	/// </summary>
	/// <returns></returns>
	inline double getNumericalValue(std::variant<double, std::string> val)
	{
		// Convert type to number
		if (std::holds_alternative<std::string>(val))
		{
			return std::stod(std::get<std::string>(val));
		}
		else
			return std::get<double>(val);
	}

	// Stack overflow
	inline bool ichar_equals(char a, char b)
	{
		return std::tolower(static_cast<unsigned char>(a)) ==
			std::tolower(static_cast<unsigned char>(b));
	}

	inline bool iequals(std::string_view lhs, std::string_view rhs)
	{
		return std::ranges::equal(lhs, rhs, ichar_equals);
	}

	/// <summary>
	/// Allocates a value on an altHeap, and then returns the pointer
	/// </summary>
	template <typename T>
	[[nodiscard]] inline static T* altAlloc(T value, std::vector<std::any>& altheap)
	{
		altheap.push_back(std::make_shared<T>(value));
		return std::any_cast<std::shared_ptr<T>>(altheap.back()).get();
	}
	
	/// <summary>
	/// Gives a pointer to a smart pointer within an alt heap
	/// </summary>
	template <typename T>
	[[nodiscard]] inline static T* altStore(T* ptr, std::vector<std::any>& altheap)
	{
		altheap.push_back(std::shared_ptr<T>(ptr));
		return std::any_cast<std::shared_ptr<T>>(altheap.back()).get();
	}
	/// <summary>
	/// Evaluates a value as a bool
	/// </summary>
	inline bool toBoolean(std::variant<double, std::string> val)
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
						throw;
				}
				return std::stod(str) >= 1;
			}
		}
		else
		{
			return std::get<double>(val) >= 1;
		}
	}

	/// <summary>
	/// Sets up the required variables for live interpreting
	/// </summary>
	void liveIntrepretSetup();
	/// <summary>
	/// Interprets in live cli session
	/// </summary>
	void liveIntrepret(std::shared_ptr<ast::Expression> expr);
	/// <summary>
	/// Interprets ast tree
	/// </summary>
	/// <param name="astTree">Ast tree to be interpreted</param>
	void interpret(std::shared_ptr<ast::Expression> expr, int argc, char** argv);
	/// <summary>
	/// Evaluates ast tree and adds all of an it's symbols to another symbol table
	/// </summary>
	/// <param name="expr"></param>
	void include(std::shared_ptr<ast::Expression> expr, SymbolTable* symtab, ArgState& argState);
	/// <summary>
	/// Returns the value of a member, derived from it's contained expression and other values.
	/// </summary>
	/// <param name="member">Member to evaluate</param>
	/// <param name="write">Whether or not to write the evaluated value down. Function calls need to be able to repeatedly evaluate</param>
	/// <returns>An std::variant<double, std::string> representing the ultimate value of the object</returns>
	std::variant<double, std::string> evaluate(objectOrValue member, SymbolTable* symtab, ArgState& args, bool write = true);
	/// <summary>
	/// Returns the value of a member, derived from it's contained expression and other values. 
	/// Does not double evaluate, making it not suitable for member functions. Otherwise identical to evaluate
	/// </summary>
	/// <param name="member">Member to evaluate</param>
	/// <param name="write">Whether or not to write the evaluated value down. Function calls need to be able to repeatedly evaluate</param>
	/// <returns>An std::variant<double, std::string> representing the ultimate value of the object</returns>
	objectOrValue softEvaluate(objectOrValue member, SymbolTable* symtab, ArgState& args, bool write = true); // TODO: Better name
	/// <summary>
	/// Calls object
	/// </summary>
	/// <param name="object"></param>
	std::variant<double, std::string> callObject(objectOrValue member, SymbolTable* symtab, ArgState& argState, std::vector<objectOrValue> args = {});
	/// <summary>
	/// Interprets ast tree, and returns everything printed to cout
	/// </summary>
	/// <param name="astTree">Ast tree to be interpreted</param>
	/// <returns>String list containing everything printed to cout</returns>
	std::string* interpretAndReturn(std::shared_ptr<ast::Expression> expr);
	/// <summary>
	/// Adds a string to the captured cout vector
	/// </summary>
	/// <returns></returns>
	void captureString(std::string str);
	/// <summary>
	/// Returns whether or not string capture is on
	/// </summary>
	/// <returns></returns>
	bool isCapture();
}
