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
			{"Assign", Assign}
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
	/// Root symbol table of the program
	/// </summary>
	static SymbolTable globalSymtab;
	/// <summary>
	/// Internal recursive function for interpreting an ast tree
	/// </summary>
	/// <param name="expr">Ast node to interpret</param>
	/// <param name="call">Whether or not to evaluate call values</param>
	/// <returns>The value of the node</returns>
	const objectOrValue interpret_interal(ast::Expression* expr, SymbolTable* symtab, bool call);
	/// <summary>
	/// Calls object
	/// </summary>
	/// <param name="object"></param>
	void call(Object* object, SymbolTable* symtab);

	void captureString(std::string str)
	{
		capturedCout.push_back(str);
	}

	std::string* interpretAndReturn(ast::Expression* expr)
	{
		clearSymtab(globalSymtab);
		capture = true;
		capturedCout.clear();
		interpret_interal(expr, &globalSymtab, true);
		std::shared_ptr<Object> main = std::get<std::shared_ptr<Object>>(globalSymtab.lookUp("Main"));
		call(main.get(), &globalSymtab);
		return capturedCout.data();
	}

	void interpret(ast::Expression* expr)
	{
		capture = false;
		interpret_interal(expr, &globalSymtab, false);
	}

	const objectOrValue interpret_interal(ast::Expression* expr, SymbolTable* symtab, bool call)
	{
		if (dynamic_cast<ast::Identifier*>(expr) != nullptr)
		{
			auto node = dynamic_cast<ast::Identifier*>(expr);
			return std::get<std::shared_ptr<Object>>(symtab->lookUp(node->name)); // :pray: :sob:
		}
		else if (dynamic_cast<ast::Literal*>(expr) != nullptr)
		{
			auto node = dynamic_cast<ast::Literal*>(expr);
			return node->litValue;
		}
		else if (dynamic_cast<ast::Call*>(expr) != nullptr)
		{
			auto node = dynamic_cast<ast::Call*>(expr);
			objectOrBuiltin called = symtab->lookUp(node->name);
			// Get arguments	
			std::vector<objectOrValue> args;
			for (auto arg : node->args)
			{
				// Do not evaluate here
				args.push_back(interpret_interal(arg, symtab, false));

				// Even if not called, this might still be useful for evaluation purposes
				// Evaluate the situtation (pun intended)
			}

			if (call)
			{
				if (std::holds_alternative<BuiltIn>(called)) // Call built-in
				{
					return std::get<BuiltIn>(called)(args, symtab);
				}
				else // Call Object
				{
					SymbolTable localSt = SymbolTable(symtab); // Going down in scope
					throw; // TODO
				}
			}
			else
			{
				return std::make_shared<Object>(node->name, node);
			}
		}
		else if (dynamic_cast<ast::BinaryOperator*>(expr) != nullptr)
		{
			throw; //TODO
		}
		else if (dynamic_cast<ast::UnaryOperator*>(expr) != nullptr)
		{
			throw; //TODO
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
				auto r = interpret_interal(object.get()->getExpression(), symtab, true);
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

	void call(Object* object, SymbolTable* symtab)
	{
		for (auto arg : object->getMembers())
		{
			evaluate(*arg, symtab);
		}
	}
}