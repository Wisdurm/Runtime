#include "interpreter.h"
#include "Lib/StandardLibrary.h"
// C++
#include <vector>

namespace rt
{
	// Symbol table
	symbol* SymbolTable::lookUp(std::string key)
	{
		// Check if key exists
		std::unordered_map<std::string, symbol>::iterator it = locals.find(key);
		if (it != locals.end()) // Exists
			return &(it->second);

		auto p = parent;
		while (p != nullptr) // Look for key in parent symbol table
		{
			// Check if key exists
			std::unordered_map<std::string, symbol>::iterator it = p->locals.find(key);
			if (it != p->locals.end()) // Exists
				return &(it->second);
			else
				p = p->parent;
		}

		// Cannot find
		// Create symbol ( possibly shadow built in functions? undefined behaviour atm )
		// TODO: THIS SHOULD GET A REFERENCE TO A PARAMETERS IF SUCH EXISTS IN CURRENT SCOPE
		Object zero = Object(key);
		updateSymbol(key, zero);
		return lookUp(key);
	}

	void SymbolTable::updateSymbol(const std::string& key, const Object& object)
	{
		// Check if key exists
		std::unordered_map<std::string, symbol>::iterator it = locals.find(key);
		if (it != locals.end()) // Exists
			it->second = Object(object); // I don't think there's any chance this works but I'm a bit tired atm so I'll figure this out in the coming days/weeks
		else
			locals.insert({ key, object });
	}

	void SymbolTable::clear()
	{
		locals.clear();
		if (parent != nullptr)
			parent->clear();
	}

	// Interpreter

	/// <summary>
	/// Whether or not to capture cout to string list
	/// </summary>
	static bool capture;
	bool isCapture() { return capture; };
	/// <summary>
	/// Strings captured from cout
	/// </summary>
	static std::vector<std::string> capturedCout = {};
	/// <summary>
	/// Root symbol table of the program
	/// </summary>
	static SymbolTable symtab = SymbolTable({ {"Print", Print } });
	/// <summary>
	/// Internal recursive function for interpreting an ast tree
	/// </summary>
	/// <param name="expr">Ast node to interpret</param>
	/// <returns>The value of the node</returns>
	const member& interpret_interal(ast::Expression* expr, SymbolTable symtab);

	void captureString(std::string str)
	{
		capturedCout.push_back(str);
	}

	std::string* interpretAndReturn(ast::Expression* expr)
	{
		capture = true;
		capturedCout.clear();
		interpret_interal(expr, symtab);
		return capturedCout.data();
	}

	void interpret(ast::Expression* expr)
	{
		capture = false;
		interpret_interal(expr, symtab);
	}

	const member& interpret_interal(ast::Expression* expr, SymbolTable symtab)
	{
		if (dynamic_cast<ast::Identifier*>(expr) != nullptr)
		{
			auto node = dynamic_cast<ast::Identifier*>(expr);
			return std::get<Object>(*symtab.lookUp(node->name));
		}
		else if (dynamic_cast<ast::Literal*>(expr) != nullptr)
		{
			auto node = dynamic_cast<ast::Literal*>(expr);
			return node->value;
		}
		else if (dynamic_cast<ast::Call*>(expr) != nullptr)
		{
			auto node = dynamic_cast<ast::Call*>(expr);
			symbol called = *symtab.lookUp(node->name);
			if (std::holds_alternative<BuiltIn>(called)) // Call built-in
			{
				std::vector<Object> args = {};
				// TODO bruhhhh
				return std::get<BuiltIn>(called)(args);
			}
			else // Call Object
			{
				SymbolTable localSt = SymbolTable(&symtab); // Going down in scope

			}
		}
		else if (dynamic_cast<ast::BinaryOperator*>(expr) != nullptr)
		{

		}
		else if (dynamic_cast<ast::UnaryOperator*>(expr) != nullptr)
		{

		}
		else
			throw;
	}
}