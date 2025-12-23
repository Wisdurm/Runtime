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
#include <cstdlib>
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

	// TODO: ORGANIZE CODE OH MY DAYS

	// Source - https://stackoverflow.com/a
	// Posted by Andrew Top
	// Retrieved 2025-12-20, License - CC BY-SA 2.5

	template <class T>
	class Traits
	{
	public:
		struct AlignmentFinder
		{
			char a; 
			T b;
		};

		enum {AlignmentOf = sizeof(AlignmentFinder) - sizeof(T)};
	};
	/// <summary>
	/// Stores all currently loaded shared libraries
	/// </summary>
	static std::vector<void*> libraries;
	/// <summary>
	/// The alignment of all ffi_types
	/// </summary>
	static const std::unordered_map<ffi_type*, size_t> typeAlignments = {
		{&ffi_type_uint8, Traits<uint8_t>::AlignmentOf},
		{&ffi_type_sint8, Traits<int8_t>::AlignmentOf},
		{&ffi_type_uint16, Traits<uint16_t>::AlignmentOf},
		{&ffi_type_sint16, Traits<int16_t>::AlignmentOf},
		{&ffi_type_uint32, Traits<uint32_t>::AlignmentOf},
		{&ffi_type_sint32, Traits<int32_t>::AlignmentOf},
		{&ffi_type_uint64, Traits<uint64_t>::AlignmentOf},
		{&ffi_type_sint64, Traits<int64_t>::AlignmentOf},
		{&ffi_type_float, Traits<float>::AlignmentOf},
		{&ffi_type_double, Traits<double>::AlignmentOf},
		{&ffi_type_uchar, Traits<unsigned char>::AlignmentOf},
		{&ffi_type_schar, Traits<signed char>::AlignmentOf},
		{&ffi_type_ushort, Traits<unsigned short>::AlignmentOf},
		{&ffi_type_sshort, Traits<signed short>::AlignmentOf},
		{&ffi_type_uint, Traits<unsigned int>::AlignmentOf},
		{&ffi_type_sint, Traits<signed int>::AlignmentOf},
		{&ffi_type_ulong, Traits<unsigned long>::AlignmentOf},
		{&ffi_type_slong, Traits<signed long>::AlignmentOf},
		{&ffi_type_longdouble, Traits<long double>::AlignmentOf},
		{&ffi_type_pointer, Traits<void*>::AlignmentOf},
		// TODO: Complex
	};
	/// <summary>
	/// The size of all ffi_types
	/// </summary>
	static const std::unordered_map<ffi_type*, size_t> typeSizes = {
		{&ffi_type_uint8, sizeof(uint8_t)},
		{&ffi_type_sint8, sizeof(int8_t)},
		{&ffi_type_uint16, sizeof(uint16_t)},
		{&ffi_type_sint16, sizeof(int16_t)},
		{&ffi_type_uint32, sizeof(uint32_t)},
		{&ffi_type_sint32, sizeof(int32_t)},
		{&ffi_type_uint64, sizeof(uint64_t)},
		{&ffi_type_sint64, sizeof(int64_t)},
		{&ffi_type_float, sizeof(float)},
		{&ffi_type_double, sizeof(double)},
		{&ffi_type_uchar, sizeof(unsigned char)},
		{&ffi_type_schar, sizeof(signed char)},
		{&ffi_type_ushort, sizeof(unsigned short)},
		{&ffi_type_sshort, sizeof(signed short)},
		{&ffi_type_uint, sizeof(unsigned int)},
		{&ffi_type_sint, sizeof(signed int)},
		{&ffi_type_ulong, sizeof(unsigned long)},
		{&ffi_type_slong, sizeof(signed long)},
		{&ffi_type_longdouble, sizeof(long double)},
		{&ffi_type_pointer, sizeof(void*)},
		// TODO: Complex
	};

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
			globalSymtab.updateSymbol(sym, LibFunc(fptr, false, std::experimental::make_observer<ffi_type>(nullptr), {}));
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

	/// <summary>
	/// Adds a shared ptr of a specified ffi_type to a vector
	/// </summary>
	static void addSharedType(std::vector<std::any>& arguments, ffi_type* const& type, std::variant<double, std::string> value)
	{
		/*{{{*/
		if (type == &ffi_type_uint8)
			// Shared because vector requires copyable types
			arguments.push_back(std::make_shared<uint8_t>(getNumericalValue(value)));
		else if (type == &ffi_type_sint8)
			arguments.push_back(std::make_shared<int8_t>(getNumericalValue(value)));
		else if (type == &ffi_type_uint16)
			arguments.push_back(std::make_shared<uint16_t>(getNumericalValue(value)));
		else if (type == &ffi_type_sint16)
			arguments.push_back(std::make_shared<int16_t>(getNumericalValue(value)));
		else if (type == &ffi_type_uint32)
			arguments.push_back(std::make_shared<uint32_t>(getNumericalValue(value)));
		else if (type == &ffi_type_sint32)
			arguments.push_back(std::make_shared<int32_t>(getNumericalValue(value)));
		else if (type == &ffi_type_uint64)
			arguments.push_back(std::make_shared<uint64_t>(getNumericalValue(value)));
		else if (type == &ffi_type_sint64)
			arguments.push_back(std::make_shared<int64_t>(getNumericalValue(value)));
		else if (type == &ffi_type_float)
			arguments.push_back(std::make_shared<float>(getNumericalValue(value)));
		else if (type == &ffi_type_double)
			arguments.push_back(std::make_shared<double>(getNumericalValue(value)));
		else if (type == &ffi_type_uchar)
			arguments.push_back(std::make_shared<unsigned char>(getNumericalValue(value)));
		else if (type == &ffi_type_schar)
			arguments.push_back(std::make_shared<signed char>(getNumericalValue(value)));
		else if (type == &ffi_type_ushort)
			arguments.push_back(std::make_shared<unsigned short>(getNumericalValue(value)));
		else if (type == &ffi_type_sshort)
			arguments.push_back(std::make_shared<short>(getNumericalValue(value)));
		else if (type == &ffi_type_uint)
			arguments.push_back(std::make_shared<unsigned int>(getNumericalValue(value)));
		else if (type == &ffi_type_sint)
			arguments.push_back(std::make_shared<int>(getNumericalValue(value)));
		else if (type == &ffi_type_ulong)
			arguments.push_back(std::make_shared<unsigned long>(getNumericalValue(value)));
		else if (type == &ffi_type_slong)
			arguments.push_back(std::make_shared<long>(getNumericalValue(value)));
		else if (type == &ffi_type_longdouble)
			arguments.push_back(std::make_shared<long double>(getNumericalValue(value)));
		else if (type == &ffi_type_cstring)
			arguments.push_back(std::make_shared<char*>(std::get<std::string>(value).data()));
		// TODO: Allow passing pointers, then update the objects with the values of
		// the pointers after the function has been called
		/* else if (type == &ffi_type_pointer) */
		/* 	arguments.push_back(std::make_shared<void *>(getNumericalValue(value))); */
		else throw InterpreterException("Unimplemented arg type", 0, "Unknown");
		// TODO: Other types
		/*}}}*/
	}

	static objectOrValue callShared(const std::vector<objectOrValue>& args, const LibFunc& func, SymbolTable* symtab, ArgState& argState)
	{
		// TODO: Windows
		if (not func.initialized)
			throw InterpreterException("Shared function is not yet bound", 0, "Unknown");
		ffi_cif cif; // Function signature
		const int narms = func.argTypes.size(); // n params
		// Get argument types
		ffi_type** params = new ffi_type*[narms]; // Array of pointers
												  // Sometimes normal pointers are 
												  // easier than smart pointers...
		for (int i = 0; i < narms; ++i) {
			if (std::holds_alternative<std::shared_ptr<ffi_type>>(func.argTypes.at(i)))
				params[i] = std::get<std::shared_ptr<ffi_type>>(func.argTypes.at(i)).get();
			else
				params[i] = std::get<std::experimental::observer_ptr<ffi_type>>(func.argTypes.at(i)).get();
		}
		// Create CIF
		if (ffi_prep_cif(&cif, FFI_DEFAULT_ABI, narms,
					std::get<std::experimental::observer_ptr<ffi_type>>(func.retType).get(), // TODO
					params) != FFI_OK)
			throw InterpreterException("Unable to prepare cif. Likely incorrect arguments or unimplemented features.", 0, "Unknown");
		// Return value
		// TODO: Allocate actual amount of memory needed
		const auto rT = std::get<std::experimental::observer_ptr<ffi_type>>(func.retType).get();
		void* ret;
		if (rT != &ffi_type_void) // Void doesnt need memory allocated
			ret = new char[typeSizes.at(rT)];
		// Arguments
		std::vector<std::any> arguments;
		// Get arguments
		for (int i = 0; i < narms; ++i) {
			// Cast arg to type wanted by lib
			ffi_type* type = params[i];
			// First check for struct type, since then we cant
			// immediately evaluate the argument
			if (type->type == FFI_TYPE_STRUCT) {
				// Get members of arg
				// THIS IS ALL ASSUMING THERES NO packed ATTRIBUTE
				auto members = std::get<std::shared_ptr<Object>>(args.at(i))->getMembers(); // TODO: error handling
				// Create struct
				void* structMem = std::aligned_alloc(type->alignment, type->size); // TODO MEMORY LEAK LOOL
				// Assign members
				uint8_t* p = static_cast<uint8_t*>(structMem); // Move a byte at a time
				for (int j = 0, totSize = 0; type->elements[j] != NULL; ++j) {
					// Loop through member types
					const auto value = VALUEHELD(members.at(j));
					const auto t = type->elements[j];
					// DEBUG TODO
					// TODO RECURSION LOL HJAHAHAHAHAHHHAHHHH :sob:
					double val = std::get<double>(value); // HARDCODED DEBUG TODO
					// Depending on type :( once again...
					const size_t size = typeSizes.at(t);
					const size_t alignment = typeAlignments.at(t);
					const int pos = totSize + (totSize%alignment); // Align position to next block of preferred alignment
					if (t == &ffi_type_sint) 
						*reinterpret_cast<int*>(p+pos) = static_cast<int>(val);
					else if (t == &ffi_type_uchar) 
						*reinterpret_cast<unsigned char*>(p+pos) = static_cast<unsigned char>(val);
					else if (t == &ffi_type_float) 
						*reinterpret_cast<float*>(p+pos) = static_cast<float>(val);
					totSize += size + (totSize%alignment);
				}
				arguments.push_back(structMem); // HEEELPP
			}
			else { // Not struct, feel free to evaluate
				// Get value of arg
				auto value = VALUEHELD(args.at(i));
				addSharedType(arguments, type, value);
			}
		}
		// Turn custom ffi_types into real ones
		for (int i = 0; i < narms; ++i) {
			if (params[i] == &ffi_type_cstring)
				params[i] = &ffi_type_pointer;
		}
		// Void pointer array of length narms
		std::unique_ptr<void* []> call_args; // Generic pointers to args
		call_args = std::make_unique<void* []>(narms); // Create list to arg pointers
		// Pointer shenanigans
		for (int i = 0; i < narms; ++i) {
			// nightmare nightmare nightmare nightmare nightmare nightmare nightmare 
			// This is EXTREMELY prone to errors, errors which are likely quite hard to spot
			{
			/*{{{*/
			if (arguments.at(i).type() == typeid(std::shared_ptr<uint8_t>))
				call_args[i] = std::any_cast<std::shared_ptr<uint8_t>>(arguments.at(i)).get();
			else if (arguments.at(i).type() == typeid(std::shared_ptr<int8_t>))
				call_args[i] = std::any_cast<std::shared_ptr<int8_t>>(arguments.at(i)).get();
			else if (arguments.at(i).type() == typeid(std::shared_ptr<uint16_t>))
				call_args[i] = std::any_cast<std::shared_ptr<uint16_t>>(arguments.at(i)).get();
			else if (arguments.at(i).type() == typeid(std::shared_ptr<int16_t>))
				call_args[i] = std::any_cast<std::shared_ptr<int16_t>>(arguments.at(i)).get();
			else if (arguments.at(i).type() == typeid(std::shared_ptr<uint32_t>))
				call_args[i] = std::any_cast<std::shared_ptr<uint32_t>>(arguments.at(i)).get();
			else if (arguments.at(i).type() == typeid(std::shared_ptr<int64_t>))
				call_args[i] = std::any_cast<std::shared_ptr<int64_t>>(arguments.at(i)).get();
			else if (arguments.at(i).type() == typeid(std::shared_ptr<float>))
				call_args[i] = std::any_cast<std::shared_ptr<float>>(arguments.at(i)).get();
			else if (arguments.at(i).type() == typeid(std::shared_ptr<double>))
				call_args[i] = std::any_cast<std::shared_ptr<double>>(arguments.at(i)).get();
			else if (arguments.at(i).type() == typeid(std::shared_ptr<unsigned char>))
				call_args[i] = std::any_cast<std::shared_ptr<unsigned char>>(arguments.at(i)).get();
			else if (arguments.at(i).type() == typeid(std::shared_ptr<signed char>))
				call_args[i] = std::any_cast<std::shared_ptr<signed char>>(arguments.at(i)).get();
			else if (arguments.at(i).type() == typeid(std::shared_ptr<unsigned short>))
				call_args[i] = std::any_cast<std::shared_ptr<unsigned short>>(arguments.at(i)).get();
			else if (arguments.at(i).type() == typeid(std::shared_ptr<short>))
				call_args[i] = std::any_cast<std::shared_ptr<short>>(arguments.at(i)).get();
			else if (arguments.at(i).type() == typeid(std::shared_ptr<unsigned int>))
				call_args[i] = std::any_cast<std::shared_ptr<unsigned int>>(arguments.at(i)).get();
			else if (arguments.at(i).type() == typeid(std::shared_ptr<int>))
				call_args[i] = std::any_cast<std::shared_ptr<int>>(arguments.at(i)).get();
			else if (arguments.at(i).type() == typeid(std::shared_ptr<unsigned long>))
				call_args[i] = std::any_cast<std::shared_ptr<unsigned long>>(arguments.at(i)).get();
			else if (arguments.at(i).type() == typeid(std::shared_ptr<long>))
				call_args[i] = std::any_cast<std::shared_ptr<long>>(arguments.at(i)).get();
			else if (arguments.at(i).type() == typeid(std::shared_ptr<long double>))
				call_args[i] = std::any_cast<std::shared_ptr<long double>>(arguments.at(i)).get();
			else if (arguments.at(i).type() == typeid(std::shared_ptr<void*>))
				call_args[i] = std::any_cast<std::shared_ptr<void*>>(arguments.at(i)).get();
			else if (arguments.at(i).type() == typeid(std::shared_ptr<char*>)) // C string
				call_args[i] = std::any_cast<std::shared_ptr<char*>>(arguments.at(i)).get(); 
			else if (arguments.at(i).type() == typeid(void*)) // Struct
				call_args[i] = std::any_cast<void*>(arguments.at(i));
			else throw InterpreterException("Unimplemented arg pointer", 0, "Unknown");
			/*}}}*/
			}
		}
		// Call
		ffi_call(&cif, FFI_FN(func.function), ret, call_args.get());
		delete[] params;
		// Return value
		if (const auto rType = std::get_if<std::experimental::observer_ptr<ffi_type>>(&func.retType)) {
			double retVal;
			if (rType->get() == &ffi_type_void)
				return True; // Return True directly, since the return buffer has not had memory allocated here
			else if (rType->get() == &ffi_type_sint) // TODO: The rest... :/
				retVal = static_cast<double>(*static_cast<int*>(ret));
			else if (rType->get() == &ffi_type_float) 
				retVal = static_cast<double>(*static_cast<float*>(ret));
			else if (rType->get() == &ffi_type_double) 
				retVal = *static_cast<double*>(ret);
			else {
				delete[] reinterpret_cast<char*>(ret);
				throw InterpreterException("Unimplemented return type", 0, "Unknown");
			}
			// Return value, and free memory
			delete[] reinterpret_cast<char*>(ret);
			return retVal;
		} else throw; // TODO return structs :(
	}
}
