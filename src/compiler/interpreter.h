#pragma once

#include "ast.h"
#include <tsl/ordered_map.h>
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
	class ArgState;
};

/// <summary>
/// Object member type
/// </summary>
typedef std::variant<std::shared_ptr<rt::Object>, std::variant<double, std::string>> objectOrValue;
/// <summary>
/// Built-in Runtime function
/// </summary>
typedef std::function<objectOrValue(std::vector<objectOrValue>&, rt::SymbolTable*, rt::ArgState&)> BuiltIn;
/// <summary>
/// Type which symbol table points to. Either object, or a function object referencing a built in function.
/// </summary>
typedef std::variant<std::shared_ptr<rt::Object>, BuiltIn> objectOrBuiltin;

namespace rt
{
	/// <summary>
	/// Sets up the required variables for live interpreting
	/// </summary>
	void liveIntrepretSetup();
	/// <summary>
	/// Interprets in live cli session
	/// </summary>
	void liveIntrepret(ast::Expression* expr);
	/// <summary>
	/// Interprets ast tree
	/// </summary>
	/// <param name="astTree">Ast tree to be interpreted</param>
	void interpret(ast::Expression* expr);
	/// <summary>
	/// Evaluates ast tree and adds all of an it's symbols to another symbol table
	/// </summary>
	/// <param name="expr"></param>
	void include(ast::Expression* expr, SymbolTable* symtab, ArgState& argState);
	/// <summary>
	/// Returns the value of a member, derived from it's contained expression and other values.
	/// </summary>
	/// <param name="member">Member to evaluate</param>
	/// <param name="write">Whether or not to write the evaluated value down. Function calls need to be able to repeatedly evaluate</param>
	/// <returns>An std::variant<double, std::string> representing the ultimate value of the object</returns>
	std::variant<double, std::string> evaluate(objectOrValue member, SymbolTable* symtab, ArgState& args, bool write);
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
		/// Creates an empty unnamed object with a single value as a member
		/// </summary>
		/// <param name="value"></param>
		Object(std::variant<double, std::string>& value)
		{
			name = "";
			expr = nullptr;
			addMember(value);
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
		objectOrValue* getMember(int key) 
		{
			// If exists, return
			tsl::ordered_map<int, objectOrValue>::iterator it = members.find(key);
			if (it != members.end()) // Exists
				return &it.value();
			// if not exist, create
			addMember(key);
			return getMember(key);
		};
		/// <summary>
		/// Returns member by name
		/// </summary>
		/// <param name="key">Name</param>
		/// <returns></returns>
		objectOrValue* getMember(std::string name)
		{ 
			// If exists, return
			std::unordered_map<std::string, int>::iterator it = memberStringMap.find(name);
			if (it != memberStringMap.end()) // Exists
				return &members[it->second];
			// if not exist, create
			addMember(name);
			return getMember(name);
		}; // TODO: idfk man

		/// <summary>
		/// Returns member by key
		/// </summary>
		/// <returns></returns>
		objectOrValue* getMember(std::variant<double, std::string> key)
		{
			if (std::holds_alternative<double>(key)) // Number
			{
				int memberKey = static_cast<int>(std::get<double>(key));
				return getMember(memberKey);
			}
			else
			{
				std::string memberKey = std::get<std::string>(key); // String
				return getMember(memberKey);
			}
		}
		/// <summary>
		/// Returns a vector containing all of the members
		/// </summary>
		/// <returns></returns>
		std::vector<objectOrValue> getMembers()
		{
			std::vector<objectOrValue> r;
			for (tsl::ordered_map<int, objectOrValue>::iterator it = members.begin(); it != members.end(); ++it)
			{
				r.push_back(it->second);
			};
			return r; 
		};
		/// <summary>
		/// Add member with just int key
		/// </summary>
		/// <param name="key"></param>
		void addMember(int key)
		{
			members.insert({key, std::make_shared<Object>()});
		}
		/// <summary>
		/// Add member with just string key
		/// </summary>
		/// <param name="key"></param>
		void addMember(std::string key)
		{
			if (not members.contains(counter))
			{
				members.insert({counter, std::make_shared<Object>() });
				memberStringMap.insert({ key, counter });
				counter++;
			}
			else
			{
				// :(
				counter++;
				this->addMember(key);
			}
		}
		/// <summary>
		/// Adds member with int key
		/// </summary>
		/// <param name="member"></param>
		/// <param name="key"></param>
		void addMember(objectOrValue member, int key) { 
			if (key == counter) 
				counter++;
			members.insert({ key, member });
		};

		/// <summary>
		/// Add member with no key
		/// </summary>
		/// <param name="member"></param>
		void addMember(objectOrValue member)
		{
			if (not members.contains(counter))
			{
				members.insert({counter, member});
				counter++;
			}
			else
			{
				// :(
				counter++;
				this->addMember(member);
			}
		}
		/// <summary>
		/// Adds member with string key
		/// </summary>
		/// <param name="member"></param>
		/// <param name="key"></param>
		void addMember(objectOrValue member, std::string key) {
			if (not members.contains(counter))
			{
				members.insert({counter, member});
				memberStringMap.insert({key, counter});
				counter++;
			}
			else
			{
				// :(
				counter++;
				this->addMember(member, key);
			}
		};
		/// <summary>
		/// Sets the value of "key" to "value"
		/// </summary>
		/// <param name="key"></param>
		/// <param name="value"></param>
		void setMember(std::variant<double, std::string> key, objectOrValue& value)
		{
			// Delete old member and add new one because no assignment operator idk don't feel like figuring that out :/
			// This probably isn't very efficient, but it does put the new value at the top, which is maybe kind off
			// what it should do? I mean, I made the language, but I'm not really sure how stupid I want this to be
			if (std::holds_alternative<double>(key)) // Number
			{
				int memberKey = static_cast<int>(std::get<double>(key));
				if (members.contains(memberKey))
					members.erase(memberKey);
				addMember(value, memberKey);
			}
			else
			{
				std::string memberKey = std::get<std::string>(key); // String
				if (memberStringMap.contains(memberKey))
				{
					members.erase(memberStringMap.at(memberKey));
					memberStringMap.erase(memberKey);
				}
				addMember(value, memberKey);
			}
		}
		/// <summary>
		/// Sets the value of "key" to "value"
		/// </summary>
		/// <param name="key"></param>
		/// <param name="value"></param>
		void setMember(std::variant<double, std::string> key, objectOrValue value)
		{
			// Delete old member and add new one because no assignment operator idk don't feel like figuring that out :/
			// TODO: Probably not particularly hard to fix, at least anymore
			if (std::holds_alternative<double>(key)) // Number
			{
				int memberKey = static_cast<int>(std::get<double>(key));
				if (members.contains(memberKey))
					members.erase(memberKey);
				addMember(value, memberKey);
			}
			else
			{
				std::string memberKey = std::get<std::string>(key); // String
				if (memberStringMap.contains(memberKey))
				{
					members.erase(memberStringMap.at(memberKey));
					memberStringMap.erase(memberKey);
				}
				addMember(value, memberKey);
			}
		}
		/// <summary>
		/// Deletes the expression of this object
		/// </summary>
		void deleteExpression()
		{
			//delete expr; // NOT SURE IF THIS SHOULD BE DELETED?
			expr = nullptr;
		}
		/// <summary>
		/// Creates an object from an std::variant<double, std::string>
		/// </summary>
		/// <param name=""></param>
		/// <returns></returns>
		static std::shared_ptr<Object> objectFromValue(std::variant<double, std::string> value)
		{
			// TODO: Might not be needed, depends...
			// Remember the constructor associated with this!
			return std::make_shared<Object>(value);
		}
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
		tsl::ordered_map<int, objectOrValue> members;
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
		/// <param name="args">Arguments in current scope. If there are values here, they will be used instead of initializing a new one.</param>
		/// <returns>The value of a key, if not found will create new empty value</returns>
		objectOrBuiltin& lookUp(std::string key, ArgState& args);
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
		/// <summary>
		/// Whether or not the symbol table contains the specified key
		/// </summary>
		/// <param name="key">Key to look for</param>
		/// <returns></returns>
		bool contains(std::string& key);
	};

	/// <summary>
	/// Root symbol table of the program
	/// </summary>
	static SymbolTable globalSymtab;

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