#include "Runtime.h"
#include "interpreter.h"
#include "exceptions.h"
#include "Stlib/StandardLibrary.h"
#include "Stlib/StandardMath.h"
#include "Stlib/StandardIO.h"
// C++
#include <ffi.h>
#include <memory>
#include <stdexcept>
#include <variant>
#include <vector>
#include <unordered_set>
#include <algorithm>
#include <any>
// C
#include <dlfcn.h> // TODO: Windows?
#include <elf.h> // WINDOWS!!
#include <link.h>
#include <cxxabi.h>  // needed for abi::__cxa_demangle

namespace rt
{
	// Arg state

	objectOrValue* ArgState::getArg()
	{
		// If have args yet to be assigned
		if (static_cast<size_t>(pos) < args.size())
		{
			pos++;
			return &args.at(static_cast<size_t>(pos) - 1);
		}
		else if (parent != nullptr)
		{
			return parent->getArg();
		}
		else
			return nullptr;
	}

	// Symbol table
	Symbol& SymbolTable::lookUp(std::string key, ArgState& args)
	{
		// Check if key exists
		std::unordered_map<std::string, Symbol>::iterator it = locals.find(key);
		if (it != locals.end()) // Exists
			return it->second;

		auto p = parent;
		while (p != nullptr) // Look for key in parent symbol table
		{
			// Check if key exists
			std::unordered_map<std::string, Symbol>::iterator it = p->locals.find(key);
			if (it != p->locals.end()) // Exists
				return it->second;
			else
				p = p->parent;
		}

		// Cannot find, create symbol
#if RUNTIME_DEBUG==1
		std::cout << "Empty object initialized" << std::endl;
#endif // RUNTIME_DEBUG
		auto v = args.getArg();
		if (v != nullptr)
		{
			if (std::holds_alternative<std::shared_ptr<Object>>(*v)) // If argument is reference
				updateSymbol(key, std::get<std::shared_ptr<Object>>(*v));
			else // Argument is value
			{
				std::shared_ptr<Object> obj = std::make_shared<Object>(std::get<std::variant<double, std::string>>(*v));
				updateSymbol(key, obj);
			}
		}
		else
		{
			std::shared_ptr<Object> zero = std::make_shared<Object>(key);
			updateSymbol(key, zero);
		}
		return lookUp(key, args);
	}

	Symbol& SymbolTable::lookUpHard(std::string key)
	{
		// Check if key exists
		std::unordered_map<std::string, Symbol>::iterator it = locals.find(key);
		if (it != locals.end()) // Exists
			return it->second;

		auto p = parent;
		while (p != nullptr) // Look for key in parent symbol table
		{
			// Check if key exists
			std::unordered_map<std::string, Symbol>::iterator it = p->locals.find(key);
			if (it != p->locals.end()) // Exists
				return it->second;
			else
				p = p->parent;
		}
		// Cannot find, throw
		throw InterpreterException("Unable to find symbol", 0, "Unknown");
	}

	bool SymbolTable::contains(std::string& key) const
	{
		// Check if key exists
		if (locals.contains(key))
			return true;

		auto p = parent;
		while (p != nullptr) // Look for key in parent symbol table
		{
			// Check if key exists
			if (p->locals.contains(key)) // Exists
				return true;
			else
				p = p->parent;
		}
		return false;
	}

	void SymbolTable::updateSymbol(const std::string& key, const std::shared_ptr<Object> object)
	{
		// Check if key exists
		std::unordered_map<std::string, Symbol>::iterator it = locals.find(key);
		if (it != locals.end()) // Exists
			it->second = object; // I don't think there's any chance this works but I'm a bit tired atm so I'll figure this out in the coming days/weeks
		else
			locals.insert({ key, object });
	}

	void SymbolTable::updateSymbol(const std::string& key, LibFunc object)
	{
		// TODO: Maybe use variant
		// Check if key exists
		std::unordered_map<std::string, Symbol>::iterator it = locals.find(key);
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
		
	std::vector<std::string> SymbolTable::getKeys()
	{
		std::vector<std::string> keys;
		// Get from current
		for (auto kv : this->locals) {
			keys.push_back(kv.first);
		}
		// Get from parents
		auto p = parent;
		while (p != nullptr)
		{
			for (auto kv : p->locals) {
				keys.push_back(kv.first);
			}
		}
		return keys;
	}

	static void clearSymtab(SymbolTable& symtab)
	{
		symtab = SymbolTable({
			{"Return", Return },
			{"Print", Print },
			{"Input", Input },
			{"Object", ObjectF },
			{"Assign", Assign},
			{"Exit", Exit},
			{"Include", Include},
			{"Not", Not},
			{"If", If},
			{"While", While},
			{"Format", Format},
			{"Bind", Bind},
			/* {"GetKeys", GetKeys}, */
			{"Size", Size},
			// Math
			{"Add", Add},
			{"Minus", Minus},
			{"Multiply", Multiply},
			{"Divide", Divide},
			{"Mod", Mod},
			{"Equal", Equal},
			{"LargerThan", LargerThan},
			{"SmallerThan", SmallerThan},
			{"Sine", Sine},
			{"Cosine", Cosine},
			{"Tangent", Tangent},
			{"ArcSine", ArcSine},
			{"ArcCosine", ArcCosine},
			{"ArcTangent", ArcTangent},
			{"ArcTangent2", ArcTangent2},
			{"Power", Power},
			{"SquareRoot", SquareRoot},
			{"Floor", Floor},
			{"Ceiling", Ceiling},
			{"Round", Round},
			{"NaturalLogarithm", NaturalLogarithm},
			{"RandomDecimal", RandomDecimal},
			{"RandomInteger", RandomInteger},
			// I/O
			{"FileCreate", FileCreate},
			{"FileOpen", FileOpen},
			{"FileClose", FileClose},
			{"FileReadLine", FileReadLine},
			{"FileWrite", FileWrite},
			{"FileAppendLine", FileAppendLine},
			{"FileRead", FileRead},
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
	static objectOrValue interpret_internal(ast::Expression* expr, SymbolTable* symtab, bool call, ArgState& args);
	/// <summary>
	/// Calls a shared library function
	/// </summary>
	static objectOrValue callShared(const std::vector<objectOrValue>& args, const LibFunc& func, SymbolTable* symtab, ArgState& argState);
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
	/// <summary>
	/// Stores all currently loaded shared libraries
	/// </summary>
	static std::vector<void*> libraries;

	void liveIntrepretSetup()
	{
		memberInitialization = true;
		capture = false;
		clearSymtab(globalSymtab);
	}

	void liveIntrepret(ast::Expression* expr)
	{
		interpret_internal(expr, &globalSymtab, true, mainArgState);
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
		interpret_internal(expr, &globalSymtab, true, mainArgState);
		std::shared_ptr<Object> main = std::get<std::shared_ptr<Object>>(globalSymtab.lookUp("Main", mainArgState));
		memberInitialization = true;
		callObject(main, &globalSymtab, mainArgState);
		return capturedCout.data();
	}

	void interpret(ast::Expression* expr, int argc, char** argv)
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

	void include(ast::Expression* expr, SymbolTable* symtab, ArgState& argState)
	{
		// Don't forget "global" values before this was called
		bool prev = memberInitialization;
		memberInitialization = false;
		// Rename main to avoid conflict (I know this is a hacky workaround, but every way of doing this is hacky)
		// This could also be done in the parser step, which would probably be a lot smarter :thinking:
		auto node = dynamic_cast<ast::Call*>(expr);
		delete (node->args[0]);
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
		node->args[0] = new ast::Identifier(SourceLocation(), mainName);

		//
		interpret_internal(expr, symtab, true, argState);
		std::shared_ptr<Object> mainObject = std::get<std::shared_ptr<Object>>((*symtab).lookUp(mainName, argState));
		memberInitialization = true;
		callObject(mainObject, &globalSymtab, mainArgState);
		//
		memberInitialization = prev;
	}

	objectOrValue interpret_internal(ast::Expression* expr, SymbolTable* symtab, bool call, ArgState& argState)
	{
		if (dynamic_cast<ast::Identifier*>(expr) != nullptr)
		{
			auto node = dynamic_cast<ast::Identifier*>(expr);
			const Symbol& v = symtab->lookUp(node->name, argState);
			if (std::holds_alternative<std::shared_ptr<Object>>(v)) // Object
				return std::get<std::shared_ptr<Object>>(v);
			else
			{
				throw InterpreterException("Attempt to evaluate built-in function", node->src.getLine(), *node->src.getFile());
			}
		}
		else if (dynamic_cast<ast::Literal*>(expr) != nullptr)
		{
			auto node = dynamic_cast<ast::Literal*>(expr);
			return node->litValue;
		}
		else if (dynamic_cast<ast::Call*>(expr) != nullptr)
		{
			auto node = dynamic_cast<ast::Call*>(expr);

			//TODO: The next 20 or so lines of code suck REALLY bad and I HATE THEM VERY MUCH
			// THIS FUNCTION IS SOOOOO BAD BUT I REALLY DONT WANT TO REWRITE IT

			Symbol calledObject;
			objectOrValue callee;

			// If node->object is identifier,
			// check for builtin since interpret_internal can't handle that.
			// 
			if (dynamic_cast<ast::Identifier*>(node->object) != nullptr)
			{
				auto bn = dynamic_cast<ast::Identifier*>(node->object);
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
				// TODO:
				// I don't actually know if this code ever gets called.
				// This is the most complicated part of the project, and it's been months
				// since I wrote this so I genuinely don't remember why execution would ever
				// reach here
				std::cerr << "Hello, you've found something interesting."
					"Please report this message to me, as well as the code that caused this to appear.";

				callee = interpret_internal(node->object, symtab, false, argState);
				if (std::holds_alternative<std::shared_ptr<Object>>(callee)) // If object, call object
				{
					calledObject = std::get<std::shared_ptr<Object>>(callee);
				}
				else
					return std::get<std::variant<double, std::string>>(callee); // If value, return the value
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
		else if (dynamic_cast<ast::BinaryOperator*>(expr) != nullptr)
		{
			auto node = dynamic_cast<ast::BinaryOperator*>(expr);
			if (memberInitialization)
			{
				std::shared_ptr<Object> object = std::get<std::shared_ptr<Object>>(interpret_internal(node->left, symtab, true, argState));
				std::variant<double, std::string> member = std::get<std::variant<double, std::string>>(interpret_internal(node->right, symtab, true, argState));
				return *(object->getMember(member));
			}
			else
			{
				return interpret_internal(node->left, symtab, false, argState);
			}
		}
		else
			throw InterpreterException("Unimplemented ast node encountered", 0, "Unknown"); // Too lazy to get proper src loc for this
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
			if (object->getExpression() != nullptr) // Parse expression
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

	static std::vector<std::string> getSymbols(void* library)
	{
		std::vector<std::string> symbols;
		// Get symbols from library
		struct link_map* map = nullptr;
		dlinfo(library, RTLD_DI_LINKMAP, &map);
		Elf64_Sym* symtab = nullptr;
		char* strtab = nullptr;
		int symentries;
		// ???? straight from stack overflow
	    for (auto section = map->l_ld; section->d_tag != DT_NULL; ++section)
		{
			if (section->d_tag == DT_SYMTAB)
			{
				symtab = (Elf64_Sym *)section->d_un.d_ptr;
			}
			if (section->d_tag == DT_STRTAB)
			{
				strtab = (char*)section->d_un.d_ptr;
			}
			if (section->d_tag == DT_SYMENT)
			{
				symentries = section->d_un.d_val;
			}
		}
		int size = strtab - (char *)symtab;
		for (int k = 0; k < size / symentries; ++k)
		{
			auto sym = &symtab[k];
			// If sym is function
			if (ELF64_ST_TYPE(symtab[k].st_info) == STT_FUNC)
			{
				//str is name of each symbol
				auto str = &strtab[sym->st_name];
				symbols.push_back(str);
			}
		}
		return symbols;
	}

	[[nodiscard]] static const std::string cppDemangle(const char *abiName)
	{
		int status;    
		char *ret = abi::__cxa_demangle(abiName, 0, 0, &status);  
		const std::string r = std::string(ret);
		free(ret);
		return r;
	}

	void loadSharedLibrary(const char* fileName)
	{
		// Handle to the library
		void* handle = nullptr;
		handle = dlopen(fileName, RTLD_LAZY | RTLD_GLOBAL);
		if (!handle) {
			throw InterpreterException(dlerror(), 0, "Unknown");
		}
		std::vector<std::string> symbols = getSymbols(handle);
		// Demangle names (if C++)
		try {
			for (int i = 0; i < symbols.size(); ++i) {
				symbols[i] = cppDemangle(symbols[i].c_str());
			}
		} catch (std::logic_error _) {
			// Not C++
		}
		// Create objects
		for (auto sym : symbols) {
			void* fptr = dlsym(handle, sym.c_str());
			globalSymtab.updateSymbol(sym, LibFunc(fptr, false, nullptr, {}));
			// These are not yet able to be called, as they do not have
			// their necessary argument and return types set
			// The signature must be specified by calling Sign()
		}
		// Function pointer DEBUG
		// void (*fptr)() = (void (*)()) dlsym(handle, "test");
		// (*fptr)(); // Call function
		// 
		libraries.push_back(handle);
	}

	void cleanLibraries() 
	{
		for (auto lib : libraries) {
			dlclose(lib);
		}
		libraries.clear();
	}

	static objectOrValue callShared(const std::vector<objectOrValue>& args, const LibFunc& func, SymbolTable* symtab, ArgState& argState)
	{
		// TODO: Windows
		if (not func.initialized)
			throw InterpreterException("Shared function is not yet bound", 0, "Unknown");
		ffi_cif cif; // Function signature
		ffi_type** params = const_cast<ffi_type**>(func.argTypes.data());
		int ret;
		const int narms = func.argTypes.size(); // n params
		// Arguments
		std::vector<std::any> arguments;
		// Get arguments
		for (int i = 0; i < narms; ++i) {
			// Get value of arg
			auto value = VALUEHELD(args.at(i));
			// Cast arg to type wanted by lib
			ffi_type* type = func.argTypes.at(i);
			// :cry: :sob:
			if (type == &ffi_type_sint)
				// Shared because vector requires copyable types
				arguments.push_back(std::make_shared<int>(getNumericalValue(value)));
			else if (type == &ffi_type_float)
				arguments.push_back(std::make_shared<float>(getNumericalValue(value)));
			else throw InterpreterException("Unimplemented arg type", 0, "Unknown");
			// TODO: Other types
		}
		// Void pointer array of length narms
		std::unique_ptr<void* []> call_args; // Generic pointers to args
		call_args = std::make_unique<void* []>(narms); // Create list to arg pointers
		// Pointer shenanigans
		for (int i = 0; i < narms; ++i) {
			// nightmare nightmare nightmare nightmare nightmare nightmare nightmare 
			if (arguments.at(i).type() == typeid(std::shared_ptr<int>))
				call_args[i] = std::any_cast<std::shared_ptr<int>>(arguments.at(i)).get();
			else if (arguments.at(i).type() == typeid(std::shared_ptr<float>))
				call_args[i] = std::any_cast<std::shared_ptr<float>>(arguments.at(i)).get();
		}
		// Create CIF
		if (ffi_prep_cif(&cif, FFI_DEFAULT_ABI, 1, &ffi_type_sint, params) != FFI_OK)
			throw InterpreterException("Unable to prepare cif. Likely incorrect arguments or unimplemented features.", 0, "Unknown");
		// TODO
		// Call
		ffi_call(&cif, FFI_FN(func.function), &ret, call_args.get());
		// DEBUG DEBUG DEBUG DEBUG DEBUG DEBUG DEBUG DEBUG 
		double val = static_cast<double>(static_cast<int>(ret));
		return val;
	}
}
