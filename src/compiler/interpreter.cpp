// Runtime
#include "Runtime.hpp"
#include "interpreter.h"
#include "shared_libs.h"
#include "symbol_table.h"
#include "object.h"
#include "exceptions.h"
// C++
#include <ffi.h>
#include <memory>
#include <variant>
#include <vector>
#include <unordered_set>
#include <cstring>
// C
#include <cstdlib>

namespace rt
{
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
	static objectOrValue interpret_internal(std::shared_ptr<ast::Expression> expr, SymbolTable* symtab, bool call, ArgState& args);
	/// <summary>
	/// Whether or not members can be initialized by reference (ie. obj-0)
	/// </summary>
	static bool memberInitialization;
	/// <summary>
	/// Arguments for Main
	/// </summary>
	static std::vector<objectOrValue> mainArgs;
	/// <summary>
	/// Main argument state, all arg states should be derived from this one
	/// </summary>
	static ArgState mainArgState = ArgState(mainArgs);
	/// <summary>
	/// Stores all objects currently being evaluated, in order to stop endless loops
	/// </summary>
	static std::unordered_set<std::shared_ptr<Object>> inEvaluation;

	// TODO: ORGANIZE CODE OH MY DAYS

	void liveIntrepretSetup()
	{
		memberInitialization = true;
		capture = false;
		clearSymtab(globalSymtab);
	}

	void liveIntrepret(std::shared_ptr<ast::Expression> expr)
	{
		interpret_internal(expr, &globalSymtab, true, mainArgState);
	}

	void captureString(std::string str)
	{
		capturedCout.push_back(str);
	}

	std::string* interpretAndReturn(std::shared_ptr<ast::Expression> expr)
	{
		// Clear
		mainArgs.clear();
		mainArgState = ArgState(mainArgs);
		memberInitialization = false;
		clearSymtab(globalSymtab);
		capture = true;
		capturedCout.clear();
		// Begin
		interpret_internal(expr, &globalSymtab, true, mainArgState);
		std::shared_ptr<Object> main = std::get<std::shared_ptr<Object>>(globalSymtab.lookUp("Main", mainArgState));
		memberInitialization = true;
		callObject(main, &globalSymtab, mainArgState);
		// Clear
		cleanLibraries();
		return capturedCout.data();
	}

	void interpret(std::shared_ptr<ast::Expression> expr, int argc, char** argv)
	{
		memberInitialization = false;
		// Load stdlib
		clearSymtab(globalSymtab);
		// Load command line arguments
		for (int i = 0; i < argc; ++i) {
			std::variant<double, std::string> value = argv[i];
			std::string name = "carg";
			name += std::to_string(i);
			globalSymtab.updateSymbol(name, std::make_shared<Object>(value ));
		}
		// Get objects
		capture = false;
		interpret_internal(expr, &globalSymtab, true, mainArgState);
		std::shared_ptr<Object> main = std::get<std::shared_ptr<Object>>(globalSymtab.lookUp("Main", mainArgState));
		memberInitialization = true;
		// Run code starting from main function
		callObject(main, &globalSymtab, mainArgState);
	}

	void include(std::shared_ptr<ast::Expression> expr, SymbolTable* symtab, ArgState& argState)
	{
		// Don't forget "global" values before this was called
		bool prev = memberInitialization;
		memberInitialization = false;
		// Rename main to avoid conflict (I know this is a hacky workaround, but every way of doing this is hacky)
		// This could also be done in the parser step, which would probably be a lot smarter :thinking:
		auto node = std::dynamic_pointer_cast<ast::Call>(expr);
		int mainCounter = 2;
		std::string mainName;
		while (true)
		{
			mainName = "Main";
			mainName += std::to_string(mainCounter);
			if (not symtab->contains(mainName))
				break;
			mainCounter++;
		}
		// TODO: Is this safe?
		node->args[0] = std::make_shared<ast::Identifier>(SourceLocation(), mainName);
		//
		interpret_internal(expr, symtab, true, argState);
		std::shared_ptr<Object> mainObject = std::get<std::shared_ptr<Object>>((*symtab).lookUp(mainName, argState));
		memberInitialization = true;
		callObject(mainObject, &globalSymtab, mainArgState);
		//
		memberInitialization = prev;
	}

	objectOrValue interpret_internal(std::shared_ptr<ast::Expression> expr, SymbolTable* symtab, bool call, ArgState& argState)
	{
		if (auto node = std::dynamic_pointer_cast<ast::Identifier>(expr))
		{
			const Symbol& v = symtab->lookUp(node->name, argState);
			if (std::holds_alternative<std::shared_ptr<Object>>(v)) // Object
				return std::get<std::shared_ptr<Object>>(v);
			else
				throw InterpreterException("Attempt to evaluate built-in function", node->src.getLine(), *node->src.getFile());
		}
		else if (auto node = std::dynamic_pointer_cast<ast::Literal>(expr))
		{
			return node->litValue;
		}
		else if (auto node = std::dynamic_pointer_cast<ast::Call>(expr))
		{
			//TODO: The next 20 or so lines of code suck REALLY bad and I HATE THEM VERY MUCH
			// THIS FUNCTION IS SOOOOO BAD BUT I REALLY DONT WANT TO REWRITE IT

			Symbol calledObject;
			std::shared_ptr<Object> callee;

			// If node->object is identifier,
			// check for builtin since interpret_internal can't handle that.
			if (auto bn = std::dynamic_pointer_cast<ast::Identifier>(node->object))
			{
				const auto& v = symtab->lookUp(bn->name, argState); // Look up object in symtab
				if (std::holds_alternative<BuiltIn>(v)) // Builtin
				{
					calledObject = std::get<BuiltIn>(v);
				}
				else if (std::holds_alternative<LibFunc>(v)) // Library function
				{
					calledObject = std::get<LibFunc>(v);
				}
				else // Not builtin
				{
					calledObject = std::get<std::shared_ptr<Object>>(v);
				}
				callee = std::make_shared<Object>(bn->name, node);
			}
			else // Not builtin function
			{	
				// This gets called when calling member functions
				// I genuinely don't understand how I wrote this code, but I'm so glad
				// I managed.
				auto tmp = interpret_internal(node->object, symtab, false, argState);
				if (std::holds_alternative<std::shared_ptr<Object>>(tmp)) // If object, call object
				{
					callee = std::get<std::shared_ptr<Object>>(tmp);
					calledObject = callee;
				}
				else
					return std::get<std::variant<double, std::string>>(tmp); // If value, return the value
			}

			if (call)
			{
				// Get arguments	
				std::vector<objectOrValue> args;
				for (auto arg : node->args)
				{
					// Do not evaluate here
					args.push_back(interpret_internal(arg, symtab, false, argState));
				}

				if (std::holds_alternative<BuiltIn>(calledObject)) // Call built-in
				{
					return std::get<BuiltIn>(calledObject)(args, symtab, argState);
				}
				else if (std::holds_alternative<LibFunc>(calledObject)) // Call library
				{
					return callShared(args, std::get<LibFunc>(calledObject), symtab, argState);
				}
				else // Call Runtime Object
				{
					SymbolTable localSt = SymbolTable(symtab); // Going down in scope
					return callObject(std::get<std::shared_ptr<Object>>(calledObject), &localSt, argState, args);
				}
			}
			else
			{
				return callee;
			}
		}
		else if (auto node = std::dynamic_pointer_cast<ast::BinaryOperator>(expr))
		{
			if (memberInitialization)
			{
				std::shared_ptr<Object> object;
				std::variant<double, std::string> member;
				try {
					object = std::get<std::shared_ptr<Object>>(interpret_internal(node->left, symtab, true, argState));
				} catch (std::bad_variant_access) {
					throw InterpreterException("Left-hand operand of accession was not object", node->src.getLine(), *node->src.getFile());
				}
				try {
					member = std::get<std::variant<double, std::string>>(interpret_internal(node->right, symtab, true, argState));
				} catch (std::bad_variant_access) {
					throw InterpreterException("Right-hand operand of accession was not a value", node->src.getLine(), *node->src.getFile());
				}
				return *(object->getMember(member));
			}
			else
			{
				// This needs to return a reference to a member that doesn't exist yet
				// As a result, we give it the uninterpreted, raw ast tree, so it can be parsed
				// later, when actually possible.
				return std::make_shared<Object>(node);
			}
		}
		else
			throw InterpreterException("Unimplemented ast node encountered", expr->src.getLine(), *expr->src.getFile());
	}

	std::variant<double, std::string> evaluate(objectOrValue member, SymbolTable* symtab, ArgState& argState, bool write)
	{
		if (std::holds_alternative<std::shared_ptr<Object>>(member))
		{
			std::shared_ptr<Object> object = std::get<std::shared_ptr<Object>>(member);
			if (inEvaluation.contains(object)) { // TODO: Should maybe allow multiple ones
				inEvaluation.erase(object);
				// Get source location
				if (object->getExpression() != nullptr) {
					SourceLocation loc = object->getExpression()->src;
					throw InterpreterException("Object evaluation got stuck in an infinite loop", loc.getLine(), *loc.getFile());
				}
				else {
					// TODO: expand search
					throw InterpreterException("Object evaluation got stuck in an infinite loop", 0, "Unknown");
				}
			}
			inEvaluation.insert(object); // This is currently being evaluated
			if (object->getExpression()) // Parse expression
			{
				auto r = evaluate(interpret_internal(object->getExpression(), symtab, true, argState), symtab, argState, write);
				if (write)
				{
#if RUNTIME_DEBUG==1
				std::cout << "Value evaluated to memory";
#endif // RUNTIME_DEBUG
					object->addMember(r);
					object->deleteExpression();
				}
				inEvaluation.erase(object);
				return r;
			}
			else if (auto members = object->getMembers(); members.size() > 0)
			{
				auto r = evaluate(*(--members.end()), symtab, argState, write); // Evaluate last member
				inEvaluation.erase(object);
				return r;
			}
			else // No value, generate empty member
			{
				inEvaluation.erase(object); // Immediately remove since this one is fine to double evaluate
#if RUNTIME_DEBUG==1
				std::cout << "Empty value initialized" << std::endl;
#endif // RUNTIME_DEBUG
				objectOrValue z = 0.0;
				// This is required to activate the right constructor
				// it's a leftover of an older system (ast::value)
				// probably could be improved 
				object->addMember(z);
				auto r = evaluate(object, symtab, argState, write); // Not particularly efficient but it works
				return r;
			}
		}
		else // Value
		{
			return std::get<std::variant<double, std::string>>(member);
		}
	}

	objectOrValue softEvaluate(objectOrValue member, SymbolTable* symtab, ArgState& argState, bool write)
	{
		if (std::holds_alternative<std::shared_ptr<Object>>(member))
		{
			std::shared_ptr<Object> object = std::get<std::shared_ptr<Object>>(member);
			if (inEvaluation.contains(object)) { // TODO: Should maybe allow multiple ones
				inEvaluation.erase(object);
				// Get source location
				if (object->getExpression() != nullptr) {
					SourceLocation loc = object->getExpression()->src;
					throw InterpreterException("Object evaluation got stuck in an infinite loop", loc.getLine(), *loc.getFile());
				}
				else {
					// TODO: expand search
					throw InterpreterException("Object evaluation got stuck in an infinite loop", 0, "Unknown");
				}
			}
			inEvaluation.insert(object); // This is currently being evaluated
			if (object->getExpression()) // Parse expression
			{
				auto r = interpret_internal(object->getExpression(), symtab, true, argState);
				if (write)
				{
#if RUNTIME_DEBUG==1
				std::cout << "Value evaluated to memory";
#endif // RUNTIME_DEBUG
					object->addMember(r);
					object->deleteExpression();
				}
				inEvaluation.erase(object);
				return r;
			} else throw InterpreterException("Strict evaluation not passed", 0, "Unknown");
		}
		else // Value
		{
			return std::get<std::variant<double, std::string>>(member);
		}
	}


	std::variant<double, std::string> callObject(objectOrValue member, SymbolTable* symtab, ArgState& argState, std::vector<objectOrValue> args)
	{
		if (std::holds_alternative<std::shared_ptr<Object>>(member))
		{
			std::shared_ptr<Object> object = std::get<std::shared_ptr<Object>>(member);
			// Arguments
			ArgState newArgState = argState;
			if (!args.empty()) // If no args are passed
			{
				newArgState = ArgState(args, &argState);
			}
			// Evaluate all members, and return last one
			auto members = object->getMembers();
			if (members.size() > 0)
			{
				for (int i = 0; i < members.size() - 1; i++) // All except last one
				{
					evaluate(members[i], symtab, newArgState, false);
				}
				// Return last member
				return evaluate(members[members.size() - 1], symtab, newArgState, false);
			}
			else
			{
				// This allows the calling of pure functions
				// that is, functions with no members.
				// For example see the member function test in interpreter_tests
				// Without this, methods would not work.
				//
				// HOWEVER THIS ONLY WORKS IN THE LIVE INTERPRETER
				// SINCE OBJECTS NEVER GET CALLED THEY ONLY GET EVALUATED
				// OTHERWISE  HAHAHAHAHAHAAAAAAAAAAa
				if (object->getExpression())
					return evaluate(object, symtab, newArgState, false);
				// Add zero
#if RUNTIME_DEBUG==1
				std::cout << "Empty value initialized" << std::endl;
#endif // RUNTIME_DEBUG
				objectOrValue z = 0.0;
				object->addMember(z);
				return callObject(member, symtab, argState, args);
			}
		}
		else // If value, return value
		{
			return std::get<std::variant<double, std::string>>(member);
		}
	}
}
