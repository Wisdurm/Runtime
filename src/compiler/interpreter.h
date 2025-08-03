#pragma once

#include "ast.h"
// C++
#include <unordered_map> // Do testing later on to figure out if a normal map would be better
#include <string>
#include <memory>
#include <variant>
#include <functional>
#include <vector>
#include <algorithm>

namespace rt
{
	class Object;
	class SymbolTable;
};

/// <summary>
/// Object member type
/// </summary>
typedef std::variant<std::shared_ptr<rt::Object>, ast::value> objectOrValue;
/// <summary>
/// Built-in Runtime function
/// </summary>
typedef std::function<objectOrValue(std::vector<objectOrValue>&, rt::SymbolTable*)> BuiltIn;
/// <summary>
/// Type which symbol table points to. Either object, or a function object referencing a built in function.
/// </summary>
typedef std::variant<std::shared_ptr<rt::Object>, BuiltIn> objectOrBuiltin;

namespace rt
{
	/// <summary>
	/// Interprets ast tree
	/// </summary>
	/// <param name="astTree">Ast tree to be interpreted</param>
	void interpret(ast::Expression* expr);
	/// <summary>
	/// Returns the value of a member, derived from it's contained expression and other values.
	/// </summary>
	/// <param name="member">Member to evaluate</param>
	/// <returns>An ast::value represting the ultimate value of the object</returns>
	ast::value evaluate(objectOrValue member, SymbolTable* symtab);
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
	/// <summary>
	/// Returns whether or not string capture is on
	/// </summary>
	/// <returns></returns>
	bool isCapture();

	/// <summary>
	/// Main class for representing Runtime objects
	/// </summary>
	class Object
	{
	public:
		/// <summary>
		/// Default constructor
		/// </summary>
		Object()
		{
			name = "";
			expr = nullptr;
		}
		/// <summary>
		/// Creates object with expression
		/// </summary>
		/// <param name="expr"></param>
		Object(ast::Expression* expr)
		{
			name = "";
			this->expr = expr;
		}
		/// <summary>
		/// Creates empty object with specified name and expression
		/// </summary>
		Object(std::string name, ast::Expression* expr)
		{
			this->name = name;
			this->expr = expr;
		}
		/// <summary>
		/// Creates empty object with specified name
		/// </summary>
		Object(std::string name)
		{
			this->name = name;
			expr = nullptr;
		}

		/// <summary>
		/// Return name member
		/// </summary>
		/// <returns></returns>
		std::string* getName() { return &name; };
		/// <summary>
		/// Return expr member
		/// </summary>
		/// <returns></returns>
		ast::Expression* getExpression() const { return expr; };
		/// <summary>
		/// Returns member by index
		/// </summary>
		/// <param name="key">Index</param>
		/// <returns></returns>
		objectOrValue* getMember(int key) { return &members[key]; };
		/// <summary>
		/// Returns member by name
		/// </summary>
		/// <param name="key">Name</param>
		/// <returns></returns>
		objectOrValue* getMember(std::string name) { return &members[memberStringMap[name]]; }; // TODO: idfk man
		/// <summary>
		/// Returns a vector containing all of the members
		/// </summary>
		/// <returns></returns>
		std::vector<objectOrValue*> getMembers()
		{
			std::vector<objectOrValue*> r;
			for (std::unordered_map<int, objectOrValue>::iterator it = members.begin(); it != members.end(); ++it)
			{
				r.push_back(&it->second);
			};
			return r; 
		};
		/// <summary>
		/// Adds member with int key
		/// </summary>
		/// <param name="member"></param>
		/// <param name="key"></param>
		void addMember(objectOrValue& member, int key) { 
			if (key == counter) 
				counter++;
			members.insert({ key, member });
		};
		void addMember(objectOrValue& member)
		{
			// TODO this can fuck up due to counter shenanigans.
			// TODO: cry
			members.insert({counter, member});
			counter++;
		}
		/// <summary>
		/// Adds member with string key
		/// </summary>
		/// <param name="member"></param>
		/// <param name="key"></param>
		void addMember(objectOrValue& member, std::string key) {
			if (not members.contains(counter))
			{
				members.insert({counter, member});
				memberStringMap.insert({key, counter});
				counter++;
			}
			else
			{
				// TODO i wanna cry I don't want to implement this stupid ass way of doing things
				// Hoping I figure out something better
				throw;
			}
		};
	private:
		/// <summary>
		/// Name of the object
		/// </summary>
		std::string name;
		/// <summary>
		/// (Optional) expression value. Can be evaluated
		/// </summary>
		ast::Expression* expr;
		/// <summary>
		/// Members of the object. Can either be objects, or values
		/// </summary>
		std::unordered_map<int, objectOrValue> members;
		/// <summary>
		/// Maps strings to their placement on the members list. Definitely not the best way to implement this, but I can always change it later
		/// </summary>
		std::unordered_map<std::string, int> memberStringMap;
		/// <summary>
		/// Counts up the indexing of new members
		/// </summary>
		int counter = 0;
	};

	class SymbolTable
	{
	private:
		/// <summary>
		/// Stores the values of variables in the local scope. Also points to built in functions
		/// </summary>
		std::unordered_map<std::string, objectOrBuiltin> locals;
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
		SymbolTable(const std::unordered_map<std::string, objectOrBuiltin> locals)
		{
			parent = nullptr;
			this->locals = std::unordered_map<std::string, objectOrBuiltin>(locals); // TODO what the fuck maaan
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
		/// <returns>The value of a key, if not found will create new empty value</returns>
		objectOrBuiltin& lookUp(std::string key);
		/// <summary>
		/// Changes the value of a symbol, or adds a new one to the local scope if not found
		/// </summary>
		/// <param name="key">Name of the symbol</param>
		/// <param name="value">Value of the symbol</param>
		void updateSymbol(const std::string& key, const std::shared_ptr<rt::Object> object);
		/// <summary>
		/// Clears the symbol table
		/// </summary>
		void clear();
	};
}