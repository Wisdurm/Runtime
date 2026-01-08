#include "Runtime.hpp"
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
#include <cstring>
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
			{"Append", Append },
			{"Copy", Copy },
			{"Assign", Assign},
			{"Update", Update },
			{"Exit", Exit},
			{"Include", Include},
			{"Not", Not},
			{"If", If},
			{"While", While},
			{"Format", Format},
			{"Bind", Bind},
			{"System", System},
			{"GetKeys", GetKeys},
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
	static objectOrValue interpret_internal(std::shared_ptr<ast::Expression> expr, SymbolTable* symtab, bool call, ArgState& args);
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

	/// <summary>
	/// Stores all currently loaded shared libraries
	/// </summary>
	static std::vector<void*> libraries;
	/// <summary>
	/// ffi_types
	/// </summary>
	static const std::array<ffi_type*,20> ffiTypes = {
		&ffi_type_uint8,
		&ffi_type_sint8,
		&ffi_type_uint16,
		&ffi_type_sint16,
		&ffi_type_uint32,
		&ffi_type_sint32,
		&ffi_type_uint64,
		&ffi_type_sint64,
		&ffi_type_float,
		&ffi_type_double,
		&ffi_type_uchar,
		&ffi_type_schar,
		&ffi_type_ushort,
		&ffi_type_sshort,
		&ffi_type_uint,
		&ffi_type_sint,
		&ffi_type_ulong,
		&ffi_type_slong,
		&ffi_type_longdouble,
		&ffi_type_pointer,
	};

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
			globalSymtab.updateSymbol(sym, LibFunc{
					.function = fptr,
					.initialized = false, 
					.retType = std::experimental::make_observer<ffi_type>(nullptr),
					.argTypes = {},
					.altHeap = {},
					});
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
	/// Creates a struct in a specified area of memory based on a Runtime object
	/// </summary>
	static void structFromObject(void* structMem, std::shared_ptr<Object> obj, ffi_type type,
			std::vector<std::any>& altHeap, SymbolTable* symtab, ArgState& argState)
	{
		// Here we assume we already have all the memory we need allocated;
		// this function does not allocate any memory, we are passed the structMem
		// argument, and we are to believe that thanks to libffi's genius, everything
		// difficult is already dealt with

		// TODO: Packed support, as well as look into the libffi way of doing this
		// Get members of arg
		auto members = obj->getMembers(); // TODO: error handling
		// Assign members
		uint8_t* p = static_cast<uint8_t*>(structMem); // Move a byte at a time
		for (int j = 0, totSize = 0; type.elements[j] != NULL; ++j) {
			// Loop through member types
			const auto t = type.elements[j];
			const int pos = totSize + (totSize%t->alignment); // Align position to next block of preferred alignment
			if (t->type == FFI_TYPE_STRUCT) { // Struct
				auto member = std::get<std::shared_ptr<Object>>(members.at(j)); // TODO: Error handling
				structFromObject(p + pos, member, *t, altHeap, symtab, argState);
			} else {
				const auto value = evaluate(members.at(j), symtab, argState);
				double val = std::get<double>(value); // STRING? TODO!
				if (t == &ffi_type_sint)  // TODO TE RES
					*reinterpret_cast<int*>(p+pos) = static_cast<int>(val);
				else if (t == &ffi_type_uchar) 
					*reinterpret_cast<unsigned char*>(p+pos) = static_cast<unsigned char>(val);
				else if (t == &ffi_type_float) 
					*reinterpret_cast<float*>(p+pos) = static_cast<float>(val);
				else throw InterpreterException("Unimplemented element type", 0, "Unknown");
			}
			totSize += t->size + (totSize%t->alignment);
		}
	}

	/// <summary>
	/// Creates a Runtime object from a struct in memory
	/// </summary>
	static std::shared_ptr<Object> objectFromStruct(void* strc, ffi_type structType)
	{
		// TODO: Packed support, as well as look into the libffi way of doing this
		auto obj = std::make_shared<Object>();
		uint8_t* p = static_cast<uint8_t*>(strc); // Move a byte at a time
		for (int i = 0, totSize = 0; structType.elements[i] != NULL; ++i) {
			// Loop through member types
			const auto t = structType.elements[i];
			// TODO RECURSION LOL HJAHAHAHAHAHHHAHHHH :sob:
			const int pos = totSize + (totSize%t->alignment); // Align position to next block of preferred alignment
			// Get value
			std::variant<double, std::string> value;
			if (t == &ffi_type_sint)  // TODO TE RES
				value = static_cast<double>(*reinterpret_cast<int*>(p+pos));
			else if (t == &ffi_type_float)
				value = static_cast<double>(*reinterpret_cast<float*>(p+pos));
			else if (t->type == FFI_TYPE_STRUCT) {
				obj->addMember(objectFromStruct(p+pos, *t)); // TODO: Does this actually work as intended?
			}
			else throw InterpreterException("Unimplemented element type", 0, "Unknown");
			// Add value
			obj->addMember(value);
			// For alignment
			totSize += t->size + (totSize%t->alignment);
		}
		return obj;
	}

	[[nodiscard]] static objectOrValue callShared(const std::vector<objectOrValue>& args, const LibFunc& func, SymbolTable* symtab, ArgState& argState)
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
		// Remember which params are pointers
		ffi_type** paramsCopy = new ffi_type*[narms];
		std::copy(params, params + narms, paramsCopy);
		// Return type
		ffi_type* returnType;
		if (const auto rType = std::get_if<std::experimental::observer_ptr<ffi_type>>(&func.retType))
			returnType = rType->get();
		else
			returnType = std::get<std::shared_ptr<ffi_type>>(func.retType).get();
		// Create CIF
		if (ffi_prep_cif(&cif, FFI_DEFAULT_ABI, narms,
					returnType,
					params) != FFI_OK)
			throw InterpreterException("Unable to prepare cif. Likely incorrect arguments or unimplemented features.", 0, "Unknown");
		// Return value data
		void* ret;
		if (returnType != &ffi_type_void) { // Void doesnt need memory allocated
			if (returnType->type == FFI_TYPE_STRUCT)
				ret = new char[returnType->size]; // TODO: alligned alloc?? idk...
			else // POD
				ret = new char[returnType->size]; // TODO, pointers :/
		}
		// Arguments
		std::vector<std::any> arguments;
		// Stores smart pointers which will be deallocated at the end
		// The pointers store values that need to be stored in this scope
		// but cant be smart pointers by themselves, since they're
		// created inline, and need to pass pointers to libffi.
		// Understand?
		std::vector<std::any> altHeap;
		// Get arguments
		for (int i = 0; i < narms; ++i) {
			// Cast arg to type wanted by lib
			ffi_type* type = params[i];
			
			if (type->type == FFI_TYPE_STRUCT) { // Struct
				auto obj = std::get<std::shared_ptr<Object>>(args.at(i)); // TODO: Error handling
				// Create struct
				void* structMem = std::aligned_alloc(type->alignment, type->size);
				// Store on alt heap, so it gets deallocated at the end of the function call
				// This SHOULD work, but not 100% confident, TODO if bored
				altHeap.push_back(std::shared_ptr<void>(structMem, [](void* ptr){free(ptr);} ));
				// This function actually pushes all the the necessary data to the memory buffer
				structFromObject(structMem, obj, *type, altHeap, symtab, argState);
				arguments.push_back(structMem);
			}
			else { // Not struct, feel free to evaluate
				// Get value of arg
				auto value = evaluate(args.at(i), symtab, argState);
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
				// Pointer types
				else if (type == &ffi_type_psint)
					arguments.push_back(std::make_shared<int*>( altAlloc<int>(getNumericalValue(value), altHeap)));
				// TODO: The rest
				else throw InterpreterException("Unimplemented arg type", 0, "Unknown");
				// TODO: Other types
				/*}}}*/
			}
		}
		// Turn custom ffi_types into pointers
		for (int i = 0; i < narms; ++i) {
			if (std::find(ffiTypes.begin(), ffiTypes.end(), params[i]) == ffiTypes.end() and params[i]->type != FFI_TYPE_STRUCT)
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
			// Pointers
			else if (arguments.at(i).type() == typeid(std::shared_ptr<int*>))
				call_args[i] = std::any_cast<std::shared_ptr<int*>>(arguments.at(i)).get();
			// TODO: The rest
			else throw InterpreterException("Unimplemented arg pointer", 0, "Unknown");
			/*}}}*/
			}
		}
		// Call
		ffi_call(&cif, FFI_FN(func.function), ret, call_args.get());
		delete[] params;
		// Write pointer values back to their Runtime counterparts
		for (int i = 0; i < narms; ++i) {
			if (auto pObj = std::get_if<std::shared_ptr<Object>>(&args.at(i))) { // TODO: if optimization
				double val;
				if (paramsCopy[i] == &ffi_type_psint)
					val = **reinterpret_cast<int**>(call_args[i]);
 				// TODO: The rest
				pObj->get()->setLast(val);
			}
		}
		// Finished with params
		delete[] paramsCopy;
		// Return value
		if (returnType->type != FFI_TYPE_STRUCT) {
			double retVal;
			if (returnType == &ffi_type_void)
				return True; // Return True directly, since the return buffer has not had memory allocated here
			else if (returnType == &ffi_type_sint) // TODO: The rest... :/
				retVal = static_cast<double>(*reinterpret_cast<int*>(ret));
			else if (returnType == &ffi_type_float) 
				retVal = static_cast<double>(*reinterpret_cast<float*>(ret));
			else if (returnType == &ffi_type_double) 
				retVal = *reinterpret_cast<double*>(ret);
			else {
				delete[] reinterpret_cast<char*>(ret);
				throw InterpreterException("Unimplemented return type", 0, "Unknown");
			}
			// Return value, and free memory
			delete[] reinterpret_cast<char*>(ret);
			return retVal;
		} else { // Struct return value
			// Construct Runtime object based on struct in memory
			return objectFromStruct(ret, *returnType);
		}
	}
}
