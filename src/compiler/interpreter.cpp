#include "interpreter.h"
#include <vector>

namespace rt
{
	// Symbol table
	ast::Expression* SymbolTable::lookUp(std::string key)
	{
		// Check if key exists
		std::unordered_map<std::string, std::shared_ptr<ast::Expression>>::iterator it = locals.find(key);
		if (it != locals.end()) // Exists
			return it->second.get();
		else if (parent != nullptr) // Look for key in parent symbol table
		{
			return parent->lookUp(key);
		}
		else // Cannot find
		{
			// Create symbol
			auto expr = std::make_shared<ast::Literal>(SourceLocation(), 0);
			updateSymbol(key, expr);
		}
	}

	void SymbolTable::updateSymbol(std::string key, std::shared_ptr<ast::Expression> value)
	{
		// Check if key exists
		std::unordered_map<std::string, std::shared_ptr<ast::Expression>>::iterator it = locals.find(key);
		if (it != locals.end()) // Exists
			it->second.reset(value.get()); // I don't think there's any chance this works but I'm a bit tired atm so I'll figure this out in the coming days/weeks
		else
			locals.insert({ key, value });
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
	/// <summary>
	/// Strings captured from cout
	/// </summary>
	static std::vector<std::string> capturedCout = {};
	/// <summary>
	/// Root symbol table of the program
	/// </summary>
	static SymbolTable symtab = SymbolTable();
	/// <summary>
	/// Internal recursive function for interpreting an ast tree
	/// </summary>
	/// <param name="expr">Ast node to interpret</param>
	/// <returns>The value of the node</returns>
	ast::Expression* interpret_interal(ast::Expression* expr, SymbolTable symtab);

	void captureString(std::string str)
	{
		capturedCout.push_back(str);
	}

	std::string* interpretAndReturn(ast::Expression* expr)
	{
		capture = true;
		symtab.clear();
		capturedCout.clear();
		interpret_interal(expr, symtab);
		return capturedCout.data();
	}

	void interpret(ast::Expression* expr)
	{
		capture = false;
		symtab.clear();
		interpret_interal(expr, symtab);
	}


	ast::Expression* interpret_interal(ast::Expression* expr, SymbolTable symtab)
	{
		if (dynamic_cast<ast::Identifier*>(expr) != nullptr)
		{
			auto node = dynamic_cast<ast::Identifier*>(expr);
			return symtab.lookUp(node->name);
		}
		else if (dynamic_cast<ast::Literal*>(expr) != nullptr)
		{
			auto node = dynamic_cast<ast::Literal*>(expr);
			return new ast::Literal(node->src, node->value); // This code makes literally no sense, reminder to really think through how this should work
		}
		else if (dynamic_cast<ast::Call*>(expr) != nullptr)
		{
			SymbolTable st = SymbolTable(&symtab); // Going down in scope

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