// Runtime
#include "shared_libs.h"
#include "interpreter.h"
#include "exceptions.h"
#include "Stlib/StandardFiles.h"
#include "object.h"
#include "symbol_table.h"
#include "utils.h"
// C++
#include <any>
#include <cstdint>
#include <deque>
#include <ffi.h>
#include <memory>
#include <string>
#include <variant>
#include <vector>
#include <stdexcept>
// C
#include <dlfcn.h> // TODO: Windows?
#include <elf.h> // WINDOWS!!
#include <link.h>
#include <cxxabi.h>  // needed for abi::__cxa_demangle
#include <cassert>

namespace rt
{
	/// <summary>
	/// Stores all currently loaded shared libraries
	/// </summary>
	static std::vector<void*> libraries;
	
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

	void loadSharedLibrary(const char* fileName, SymbolTable* symtab)
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
			symtab->insert(sym, std::make_shared<LibFunc>(LibFunc{
					.function = fptr,
						.initialized = false,
						.retType = std::nullopt,
						.argTypes = {},
						}));
			// These are not yet able to be called, as they do not have
			// their necessary argument and return types set
			// The signature must be specified by calling Sign()
		}
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
	static void structFromObject(void* structMem, std::shared_ptr<Object> obj, const Type& type,
				     SymbolTable* symtab, ArgState& argState, std::deque<std::any>& altHeap)
	{
		// Here we assume we already have all the memory we need allocated;
		// this function does not allocate any memory, we are passed the structMem
		// argument, and we are to believe that thanks to libffi's genius, everything
		// difficult is already dealt with

		// TODO: Packed support, as well as look into the libffi way of doing this
		// Get members of arg
		auto members = obj->getMembers(); // TODO: error handling
		MemoryExplorer expl = MemoryExplorer(structMem, type);
		// Assign members
		for (int i = 0; i < type.members.size(); i++) {
			// Loop through member types
			const auto [memory, t] = expl[i];
			if (t.type == CType::Struct) { // Struct
				auto member = std::get<std::shared_ptr<Object>>(members.at(i)); // TODO: Error handling
				structFromObject(memory, member, t, symtab, argState, altHeap);
			} else {
				const auto value = evaluate(members.at(i), symtab, argState);
				if (auto str = std::get_if<std::string>(&value)) {
					// TODO: Maybe should be stored somewhere else? Idk
					altHeap.push_back(*str);
					*reinterpret_cast<char**>(memory) = std::any_cast<std::string&>(altHeap.back()).data();
				} else {
					double val = std::get<double>(value);
					switch (t.type)
					{
					case CType::Sint:
						if (t.pointer) {
							*reinterpret_cast<int**>(memory) = altAlloc(static_cast<int>(val), altHeap);
						} else {						
							*reinterpret_cast<int*>(memory) = static_cast<int>(val);						
						}
						break;
					case CType::Uchar:					
						if (t.pointer) {
							*reinterpret_cast<unsigned char**>(memory) = altAlloc(static_cast<unsigned char>(val), altHeap);
						} else {
							*reinterpret_cast<unsigned char*>(memory) = static_cast<unsigned char>(val);
						}
						break;
					case CType::Float:
						if (t.pointer) {
							*reinterpret_cast<float**>(memory) = altAlloc(static_cast<float>(val), altHeap);
						} else {
							*reinterpret_cast<float*>(memory) = static_cast<float>(val);
						}
						break;
					default:
						throw InterpreterException("Unimplemented element type", 0, "Unknown");
						break;
					}
				}
			}
		}
	}

	/// <summary>
	/// Creates a Runtime object from a struct in memory
	/// </summary>
	static std::shared_ptr<Object> objectFromStruct(void* strc, const Type& type)
	{
		// TODO: Packed support, as well as look into the libffi way of doing this
		auto obj = std::make_shared<Object>();
		MemoryExplorer expl = MemoryExplorer(strc, type);
		for (int i = 0; i < type.members.size(); i++) {
			// Loop through member types
			const auto [memory, t] = expl[i];
			if (t.type == CType::Struct) {
				obj->addMember(objectFromStruct(memory, t));
			} else {
				// Get value
				std::variant<double, std::string> value;
				switch (t.type)
				{
				case CType::Sint:
					value = static_cast<double>(*reinterpret_cast<int*>(memory));
					break;
				case CType::Float:
					value = static_cast<double>(*reinterpret_cast<float*>(memory));
					break;
				default:
					throw InterpreterException("Unimplemented element type", 0, "Unknown");
				}
				// Add value
				obj->addMember(value);
			}
		}
		return obj;
	}

	// Sets the values of a Runtime object based on pointers within a struct
	// struct may have custom types
	static void updateObject(void* callArg, std::shared_ptr<Object> obj, const Type& type)
	{
		// Loop through all the members and check if they are pointers
		MemoryExplorer expl = MemoryExplorer(callArg, type);
		for (int i = 0; i < type.members.size(); i++) {
			// Loop through member types
			const auto [memory, t] = expl[i];
			auto member = obj->getMembers()[i];
			// Check if pointer
			if (t.pointer or t.type == CType::Cstring) {
				// First check if string
				if (t.type == CType::Cstring) {
					char* str = *reinterpret_cast<char**>(memory);
					if (auto op = std::get_if<std::shared_ptr<Object>>(&member)) {
						(*op)->setLast(str);
					} else {
						std::get<std::variant<double, std::string>>(member) = str;
					}
					continue;
				}
				// If not string
				// Get value
				double val;
				switch (t.type)
				{
				case CType::Sint:
					val = **reinterpret_cast<int**>(memory);
					break;
				case CType::Float:
					val = **reinterpret_cast<float**>(memory);
					break;
				default:
					throw InterpreterException("Unimplemented element type", 0, "Unknown");
				}
				// Set value
				if (auto op = std::get_if<std::shared_ptr<Object>>(&member)) {
					(*op)->setLast(val);
				} else {
					std::get<std::variant<double, std::string>>(member) = val;
				}
			} else if (t.type == CType::Struct) {
				updateObject(memory, std::get<std::shared_ptr<Object>>(member), t); // TODO: Error handling for get<>
			}
			// Otherwise no need to update anything
		}
	}

	// Returns an std::any, which stores the provided value
	template <typename T>
	[[nodiscard]] std::any toAny(std::variant<double, std::string> value, bool pointer, std::deque<std::any>& altHeap)
	{
		if (pointer) {
			// Herkullista
			return altAlloc<T>(getNumericalValue(value), altHeap);
		} else {
			return static_cast<T>(getNumericalValue(value));
		}
	}

	// Returns a void pointer to the value stored in the std::any
	template <typename T>
	[[nodiscard]] void* toVoid(std::any& value, bool pointer)
	{
		if (pointer) {
			return &(std::any_cast<T*&>(value));
		} else {
			return &std::any_cast<T&>(value);
		}
	}

	[[nodiscard]] objectOrValue callShared(const std::vector<objectOrValue>& args, const LibFunc& func, SymbolTable* symtab, ArgState& argState)
	{
		// TODO: Fix broken state if fails midway
		// TODO: There are probably about 1000 memory leaks, memory mismagement and whatever errors in this code.
		// I'm not even sure where to begin honestly.
		
		// TODO: Windows
		if (not func.initialized)
			throw InterpreterException("Shared function is not yet bound", 0, "Unknown");
		
		// Stores smart pointers
		// The pointers store values that need to be stored in this scope
		// but cant be smart pointers by themselves, since their amount
		// and type is determined at runtime.
		std::deque<std::any> altHeap;

		// Function signature, created later
		ffi_cif cif;
		
		// Number of params
		const int narms = func.argTypes.size();
		
		// A list of the types of each argument
		std::vector<ffi_type*> paramTypes;
		paramTypes.reserve(narms);
		for (int i = 0; i < narms; i++) {
			paramTypes.push_back(func.argTypes.at(i).get());
		}
		
		// Return type
		ffi_type* returnType = func.retType.value().get();
		
		// Create CIF
		if (ffi_prep_cif(&cif, FFI_DEFAULT_ABI, narms,
				 returnType,
				 paramTypes.data()) != FFI_OK)
			throw InterpreterException("Unable to prepare cif. Likely incorrect arguments or unimplemented features.", 0, "Unknown");
		
		// Return value data
		void* ret;
		if (returnType != &ffi_type_void) { // Void doesnt need memory allocated
			ret = std::aligned_alloc(returnType->alignment, returnType->size);			
		}
		
		// The values of function arguments
		std::vector<std::any> arguments;
		arguments.reserve(narms); // Reserve so pointers stay valid
		
		// List of generic pointers to the values used to call the function.
		// This will actually be passed to ffi_call
		std::vector<void*> call_args;
		call_args.reserve(narms);
		
		// Push the values of arguments to arguments
		for (int i = 0; i < narms; ++i) {
			// Cast arg to type wanted by lib
			const Type& pType = func.argTypes.at(i);
			if (pType.type == CType::Struct) { // Struct
				auto obj = std::get<std::shared_ptr<Object>>(args.at(i)); // TODO: Error handling
				// Create struct
				const ffi_type* type = paramTypes.at(i);
				// TODO: Vector
				void* structMem = std::aligned_alloc(type->alignment, type->size);
				// Store on alt heap, so it gets deallocated at the end of the function call
				// This SHOULD work, but not 100% confident, TODO if bored
				altHeap.push_back(std::shared_ptr<void>(structMem, [](void* ptr){free(ptr);} ));
				// This function actually pushes all the the necessary data to the memory buffer
				structFromObject(structMem, obj, pType, symtab, argState, altHeap);
				arguments.push_back(structMem);
			} else { // Not struct, feel free to evaluate
				// Get value of arg
				auto value = evaluate(args.at(i), symtab, argState);
				
				switch (pType.type)
				{
				case CType::Void:
					throw InterpreterException("Cannot have void as param type", 0, "Unknown");
				case CType::Uint8:
					arguments.push_back(toAny<uint8_t>(value, pType.pointer, altHeap));
					break;
				case CType::Sint8:
					arguments.push_back(toAny<int8_t>(value, pType.pointer, altHeap));
					break;
				case CType::Sint:
					arguments.push_back(toAny<signed int>(value, pType.pointer, altHeap));
					break;
				case CType::Float:
					arguments.push_back(toAny<float>(value, pType.pointer, altHeap));
					break;
				case CType::Cstring:
					if (pType.pointer) {
						throw InterpreterException("Unimplemented feature", 0, "Unknown");
					} else {
						arguments.push_back(std::get<std::string>(value).data());
					}
					break;
				default:
					throw InterpreterException("Unimplemented arg type", 0, "Unknown");
				}					
			}
		}

		// This should demonstrate what in the world is happening here
		// int c = 18;
		// std::any a = c;
		// void* b = &std::any_cast<int&>(a);
		// assert(*((int*)b) == c);
		
		// Make call_args a list of void pointers to the actual values
		for (int i = 0; i < narms; ++i) {
			// nightmare nightmare nightmare 
			// This is quite prone to errors, errors which are likely quite hard to spot
			const Type& pType = func.argTypes.at(i);
			
			switch (pType.type)
			{
			case CType::Void:
				throw InterpreterException("Cannot have void as param type", 0, "Unknown");
			case CType::Uint8:
				call_args.push_back(toVoid<uint8_t>(arguments.at(i), pType.pointer));
				break;
			case CType::Sint8:
				call_args.push_back(toVoid<int8_t>(arguments.at(i), pType.pointer));
				break;
			case CType::Sint:
				call_args.push_back(toVoid<signed int>(arguments.at(i), pType.pointer));
				break;
			case CType::Float:
				call_args.push_back(toVoid<float>(arguments.at(i), pType.pointer));
				break;
			case CType::Cstring:
				if (pType.pointer) {
					throw InterpreterException("Unimplemented feature", 0, "Unknown");
				} else {
					call_args.push_back(toVoid<char>(arguments.at(i), true));
				}
				break;
			case CType::Struct:
				// TODO: ??
				call_args.push_back(std::any_cast<void*&>(arguments.at(i)));
				break;
			default:
				throw InterpreterException("Unimplemented arg type", 0, "Unknown");
			}			
		}

		assert(call_args.size() == narms);
		assert(arguments.size() == narms);
		
		// Call the function
		ffi_call(&cif, FFI_FN(func.function), ret, call_args.data());
		
		// Write pointer values back to their Runtime counterparts
		for (int i = 0; i < narms; ++i) {
			const Type& t = func.argTypes.at(i);
			// Check if struct, as they may have pointer members
			if (t.type == CType::Struct)
			{
				if (auto pObj = std::get_if<std::shared_ptr<Object>>(&args.at(i))) { // TODO: if optimization // ????
					updateObject(call_args.at(i), *pObj, t);
				} else {
					std::cout << "Value passed to struct argument! New values are not written down! Type: " << static_cast<int>(t.type) << std::endl;
				}
			}
			else if (t.pointer or t.type == CType::Cstring)
			{
				if (auto pObj = std::get_if<std::shared_ptr<Object>>(&args.at(i))) { // TODO: if optimization // ????
					// First check if string
					if (t.type == CType::Cstring) {
						pObj->get()->setLast(*reinterpret_cast<char**>(call_args.at(i)));
						continue;
					}
					// If not string
					double val;
					switch (t.type)
					{
					case CType::Sint:
						val = **reinterpret_cast<int**>(call_args.at(i));
						break;
					case CType::Float:
						val = **reinterpret_cast<float**>(call_args.at(i));
						break;
					default:
						throw InterpreterException("Unimplemented element type", 0, "Unknown");
					}
					// TODO: The rest
					pObj->get()->setLast(val);
				} else {
					std::cout << "Value passed to pointer argument! New values are not written down! Type: " << static_cast<int>(t.type) << std::endl;
				}
			}
			// Otherwise no need to update any values
		}
		
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
			return objectFromStruct(ret, func.retType.value());
		}
	}
}
