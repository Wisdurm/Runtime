#pragma once
// Runtime
#include "shared_libs.h"
#include "object.h"
#include "interpreter.h"
// C++
#include <unordered_map> // Do testing later on to figure out if a normal map would be better
#include <string>
#include <memory>
#include <variant>
#include <functional>
#include <vector>
#include <algorithm>
#include <any>

// Forward declarations
namespace rt
{
	class ArgState;
};

namespace rt {
	class SymbolTable
	{
	private:
		/// <summary>
		/// Stores the values of variables in the local scope. Also points to built in functions
		/// </summary>
		std::unordered_map<std::string, Symbol> locals;
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
		/// Initialized symbol constructor
		/// </summary>
		SymbolTable(const std::unordered_map<std::string, Symbol> locals)
		{
			parent = nullptr;
			this->locals = std::unordered_map<std::string, Symbol>(locals); // TODO what the fuck maaan
		}
		/// <summary>
		/// Parent constructor
		/// </summary>
		/// <param name="symtab"></param>
		SymbolTable(SymbolTable* parent)
		{
			this->parent = parent;
		}

		/// <summary>
		/// Looks up a key from the symbol table and its parents
		/// </summary>
		/// <param name="key">Key to look for</param>
		/// <param name="args">Arguments in current scope. If there are values here, they will be used instead of initializing a new one.</param>
		/// <returns>The value of a key, if not found will create new empty value</returns>
		Symbol& lookUp(std::string key, ArgState& args);
		/// <summary>
		/// Looks up a key from the symbol table and its parents, will not create a new one in case it doesn't find anything.
		/// </summary>
		/// <param name="key">Key to look for</param>
		/// <returns>The value of a key, throws an exception if not found</returns>
		Symbol& lookUpHard(std::string key); // cant be const :(
		/// <summary>
		/// Changes the value of a symbol, or adds a new one to the local scope if not found
		/// </summary>
		/// <param name="key">Name of the symbol</param>
		/// <param name="object">Value of the symbol</param>
		void updateSymbol(const std::string& key, const std::shared_ptr<rt::Object> object);
		/// <summary>
		/// Changes the value of a symbol, or adds a new one to the local scope if not found
		/// </summary>
		/// <param name="key">Name of the symbol</param>
		/// <param name="object">Value of the symbol</param>
		void updateSymbol(const std::string& key, LibFunc object);
		/// <summary>
		/// Clears the symbol table
		/// </summary>
		void clear();
		/// <summary>
		/// Whether or not the symbol table contains the specified key
		/// </summary>
		/// <param name="key">Key to look for</param>
		/// <returns></returns>
		bool contains(std::string& key) const;
		/// <summary>
		/// Returns all keys from the symbol table and it's parents
		/// </summary>
		/// <param name="key">Key to look for</param>
		/// <returns></returns>
		std::vector<std::string> getKeys();
	};

	/// <summary>
	/// Root symbol table of the program
	/// </summary>
	static SymbolTable globalSymtab;

	// Clears a symbol table and fills it with all of the standard library functions.
	void clearSymtab(SymbolTable& symtab);

	/// <summary>
	/// Current state of arguments in the interpreter.
	/// </summary>
	class ArgState
	{
	private:
		/// <summary>
		/// Arguments currently
		/// </summary>
		std::vector<objectOrValue> args;
		/// <summary>
		/// Position of earliest argument currently yet to be initialized
		/// </summary>
		int pos;
		/// <summary>
		/// Arguments from higher level
		/// </summary>
		ArgState* parent;
	public:
		/// <summary>
		/// Parent constructor
		/// </summary>
		ArgState(std::vector<objectOrValue>& args, ArgState* parent)
		{
			this->args = args;
			pos = 0;
			this->parent = parent;
		};
		/// <summary>
		/// Parentless constructor
		/// </summary>
		ArgState(std::vector<objectOrValue>& args)
		{
			this->args = args;
			pos = 0;
			parent = nullptr;
		};

		/// <summary>
		/// Returns a pointer to an argument, or nullptr if none exist.
		/// </summary>
		/// <returns></returns>
		objectOrValue* getArg();
	};
}
