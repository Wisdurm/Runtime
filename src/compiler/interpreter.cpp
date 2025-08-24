#include "interpreter.h"
#include "Stlib/StandardLibrary.h"
// C++
#include <vector>

namespace rt
{
	// Symbol table
	objectOrBuiltin& SymbolTable::lookUp(std::string key)
	{
		// Check if key exists
		std::unordered_map<std::string, objectOrBuiltin>::iterator it = locals.find(key);
		if (it != locals.end()) // Exists
			return it->second;

		auto p = parent;
		while (p != nullptr) // Look for key in parent symbol table
		{
			// Check if key exists
			std::unordered_map<std::string, objectOrBuiltin>::iterator it = p->locals.find(key);
			if (it != p->locals.end()) // Exists
				return it->second;
			else
				p = p->parent;
		}

		// Cannot find, create symbol
		// TODO: THIS SHOULD GET A REFERENCE TO A PARAMETERS IF SUCH EXISTS IN CURRENT SCOPE
		std::shared_ptr<Object> zero = std::make_shared<Object>(key);
		updateSymbol(key, zero);
		return lookUp(key);
	}

	void SymbolTable::updateSymbol(const std::string& key, const std::shared_ptr<Object> object)
	{
		// Check if key exists
		std::unordered_map<std::string, objectOrBuiltin>::iterator it = locals.find(key);
		if (it != locals.end()) // Exists
			it->second = object; // I don't think there's any chance this works but I'm a bit tired atm so I'll figure this out in the coming days/weeks
		else
			locals.insert({ key, object });
	}

	void SymbolTable::clear()
	{
		locals.clear();
		if (parent != nullptr)
			parent->clear();
	}

	static void clearSymtab(SymbolTable& symtab)
	{
		symtab = SymbolTable({
			{"Print", Print },
			{"Object", ObjectF },
			{"Assign", Assign},
			{"Exit", Exit},
		});
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
	/// Internal recursive function for interpreting an ast tree
	/// </summary>
	/// <param name="expr">Ast node to interpret</param>
	/// <param name="call">Whether or not to evaluate call values</param>
	/// <returns>The value of the node</returns>
	const objectOrValue interpret_internal(ast::Expression* expr, SymbolTable* symtab, bool call);
	/// <summary>
	/// Calls object
	/// </summary>
	/// <param name="object"></param>
	ast::value callObject(objectOrValue object, SymbolTable* symtab);

	/// <summary>
	/// Whether or not members can be initialized by reference (ie. obj-0)
	/// </summary>
	static bool memberInitialization;

	void liveIntrepretSetup()
	{
		memberInitialization = true;
		capture = false;
		clearSymtab(globalSymtab);
	}

	void liveIntrepret(ast::Expression* expr)
	{
		interpret_internal(expr, &globalSymtab, true);
	}

	void captureString(std::string str)
	{
		capturedCout.push_back(str);
	}

	std::string* interpretAndReturn(ast::Expression* expr)
	{
		memberInitialization = false;
		clearSymtab(globalSymtab);
		capture = true;
		capturedCout.clear();
		interpret_internal(expr, &globalSymtab, true);
		std::shared_ptr<Object> main = std::get<std::shared_ptr<Object>>(globalSymtab.lookUp("Main"));
		memberInitialization = true;
		callObject(main, &globalSymtab);
		return capturedCout.data();
	}

	void interpret(ast::Expression* expr)
	{
		memberInitialization = false;
		clearSymtab(globalSymtab);
		capture = false;
		interpret_internal(expr, &globalSymtab, true);
		std::shared_ptr<Object> main = std::get<std::shared_ptr<Object>>(globalSymtab.lookUp("Main"));
		memberInitialization = true;
		callObject(main, &globalSymtab);
	}

	const objectOrValue interpret_internal(ast::Expression* expr, SymbolTable* symtab, bool call)
	{
		if (dynamic_cast<ast::Identifier*>(expr) != nullptr)
		{
			auto node = dynamic_cast<ast::Identifier*>(expr);
			auto v = symtab->lookUp(node->name);
			if (std::holds_alternative<std::shared_ptr<Object>>(v))
				return std::get<std::shared_ptr<Object>>(v);
			else
				return nullptr;
		}
		else if (dynamic_cast<ast::Literal*>(expr) != nullptr)
		{
			auto node = dynamic_cast<ast::Literal*>(expr);
			return node->litValue;
		}
		else if (dynamic_cast<ast::Call*>(expr) != nullptr)
		{
			auto node = dynamic_cast<ast::Call*>(expr);

			objectOrBuiltin calledObject;
			objectOrValue callee;
			if (dynamic_cast<ast::Identifier*>(node->object) != nullptr) // First check for builtin since interpret_internal can't handle that
			{
				auto bn = dynamic_cast<ast::Identifier*>(node->object);
				auto v = (symtab->lookUp(bn->name));
				if (std::holds_alternative<BuiltIn>(v))
				{
					calledObject = std::get<BuiltIn>(v);
				}
				callee = std::make_shared<Object>(bn->name, node);
			}
			else // Not builtin function
			{
				callee = interpret_internal(node->object, symtab, false);
				if (std::holds_alternative<std::shared_ptr<Object>>(callee)) // If object, call object
				{
					calledObject = std::get<std::shared_ptr<Object>>(callee);
				}
				else
					return std::get<ast::value>(callee); // If value, return the value
			}

			// Get arguments	
			std::vector<objectOrValue> args;
			for (auto arg : node->args)
			{
				// Do not evaluate here
				args.push_back(interpret_internal(arg, symtab, false));

				// Even if not called, this might still be useful for evaluation purposes
				// Evaluate the situtation (pun intended)
			}

			if (call)
			{
				if (std::holds_alternative<BuiltIn>(calledObject)) // Call built-in
				{
					return std::get<BuiltIn>(calledObject)(args, symtab);
				}
				else // Call Object
				{
					SymbolTable localSt = SymbolTable(symtab); // Going down in scope
					return callObject(std::get<std::shared_ptr<Object>>(calledObject), &localSt);
				}
			}
			else
			{
				return callee;
			}
		}
		else if (dynamic_cast<ast::BinaryOperator*>(expr) != nullptr)
		{
			auto node = dynamic_cast<ast::BinaryOperator*>(expr);
			if (memberInitialization)
			{
				std::shared_ptr<Object> object = std::get<std::shared_ptr<Object>>(interpret_internal(node->left, symtab, true));
				ast::value member = std::get<ast::value>(interpret_internal(node->right, symtab, true));
				return *(object->getMember(member));
			}
			else
			{
				return interpret_internal(node->left, symtab, false);
			}
		}
		else
			throw;
	}

	ast::value evaluate(objectOrValue member, SymbolTable* symtab)
	{
		if (std::holds_alternative<std::shared_ptr<Object>>(member))
		{
			std::shared_ptr<Object> object = std::get<std::shared_ptr<Object>>(member);
			if (object.get()->getExpression() != nullptr) // Parse expression
			{
				auto r = interpret_internal(object.get()->getExpression(), symtab, true);
				return evaluate(r, symtab);
			}
			else if (auto members = object.get()->getMembers(); members.size() > 0)
			{
				return evaluate(**(--members.end()), symtab); // Evaluate last member
			}
			else
			{
				throw;
			}
		}
		else // Value
		{
			return std::get<ast::value>(member);
		}
	}

	ast::value callObject(objectOrValue member, SymbolTable* symtab)
	{
		if (std::holds_alternative<std::shared_ptr<Object>>(member))
		{
			std::shared_ptr<Object> object = std::get<std::shared_ptr<Object>>(member);
			// Evaluate all members, and return last one
			auto members = object->getMembers();
			for (int i = 0; i < members.size() - 1; i++) // All except last one
			{
				evaluate(*members[i], symtab);
			}
			// Return last member
			return evaluate(*members[members.size() - 1], symtab);
		}
		else // If value, return value
		{
			return std::get<ast::value>(member);
		}
	}
}