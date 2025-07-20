#pragma once

#include "ast.h"
// C++
#include <unordered_map> // Do testing later on to figure out if a normal map would be better
#include <string>
#include <memory>

namespace rt
{
	/// <summary>
	/// Interprets ast tree
	/// </summary>
	/// <param name="astTree">Ast tree to be interpreted</param>
	void interpret(ast::Expression* expr);
	/// <summary>
	/// Interprets ast tree, and returns everything printed to cout
	/// </summary>
	/// <param name="astTree">Ast tree to be interpreted</param>
	/// <returns>String list containing everything printed to cout</returns>
	std::string* interpretAndReturn(ast::Expression* expr);
	/// <summary>
	/// Adds a string to the captured cout vector
	/// </summary>
	/// <returns></returns>
	void captureString(std::string str);

	class SymbolTable
	{
	private:
		/// <summary>
		/// Stores the values of variables in the local scope
		/// </summary>
		std::unordered_map<std::string, std::shared_ptr<ast::Expression>> locals;
		/// <summary>
		/// Stores higher level variables
		/// </summary>
		SymbolTable* parent;
	public:
		/// <summary>
		/// Default constructor
		/// </summary>
		SymbolTable()
		{
			parent = nullptr;
		}
		/// <summary>
		/// Parent constructor
		/// </summary>
		/// <param name="symtab"></param>
		SymbolTable(SymbolTable* symtab)
		{
			parent = symtab;
		}

		/// <summary>
		/// Looks up a key from the symbol table and its parents
		/// </summary>
		/// <param name="key">Key to look for</param>
		/// <returns>The value of a key, if not found will create new empty value</returns>
		ast::Expression* lookUp(std::string key);
		/// <summary>
		/// Changes the value of a symbol, or adds a new one to the local scope if not found
		/// </summary>
		/// <param name="key">Name of the symbol</param>
		/// <param name="value">Value of the symbol</param>
		void updateSymbol(std::string key, std::shared_ptr<ast::Expression> value);
		/// <summary>
		/// Clears the symbol table
		/// </summary>
		void clear();
	};
}