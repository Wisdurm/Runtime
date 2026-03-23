// Runtime
#include "shared_libs.h"
#include "interpreter.h"
#include "exceptions.h"
#include "Stlib/StandardFiles.h"
#include "object.h"
#include "symbol_table.h"
#include "tokenizer.h"
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
	// Location from where callShared is being called.
	// Having this as a global variable for this file because it's just so much simpler
	static SourceLocation* srcLoc;
	
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
			throw InterpreterException(dlerror(), srcLoc->getLine(), srcLoc->getFile());
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
		auto members = obj->getMembers();
		MemoryExplorer expl = MemoryExplorer(structMem, type);
		// Assign members
		for (int i = 0; i < type.members.size(); i++) {
			// Loop through member types
			const auto [memory, t] = expl[i];
			if (t.type == CType::Struct) { // Struct
				if (not std::holds_alternative<std::shared_ptr<Object>>(members.at(i))) {
					throw InterpreterException("Cannot create struct from value argument", srcLoc->getLine(), srcLoc->getFile());
				}
				auto member = std::get<std::shared_ptr<Object>>(members.at(i));
				structFromObject(memory, member, t, symtab, argState, altHeap);
			} else {
				const auto value = evaluate(members.at(i), symtab, argState);
				if (auto str = std::get_if<std::string>(&value)) {
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
						throw InterpreterException("Unimplemented element type", srcLoc->getLine(), srcLoc->getFile());
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
					throw InterpreterException("Unimplemented element type", srcLoc->getLine(), srcLoc->getFile());
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
					throw InterpreterException("Unimplemented element type", srcLoc->getLine(), srcLoc->getFile());
				}
				// Set value
				if (auto op = std::get_if<std::shared_ptr<Object>>(&member)) {
					(*op)->setLast(val);
				} else {
					std::get<std::variant<double, std::string>>(member) = val;
				}
			} else if (t.type == CType::Struct) {
				if (not std::holds_alternative<std::shared_ptr<Object>>(member)) {
					throw InterpreterException("Cannot create struct from value argument", srcLoc->getLine(), srcLoc->getFile());
				}
				updateObject(memory, std::get<std::shared_ptr<Object>>(member), t);
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

	[[nodiscard]] objectOrValue callShared(const std::vector<objectOrValue>& args, const LibFunc& func, SymbolTable* symtab, ArgState& argState, SourceLocation src)
	{
		// Initialize srcLocation
		srcLoc = &src;
		
		// TODO: Windows
		if (not func.initialized)
			throw InterpreterException("Shared function is not yet bound", srcLoc->getLine(), srcLoc->getFile());
		
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
				 paramTypes.data()) != FFI_OK) {
			throw InterpreterException("Unable to prepare cif. Likely incorrect arguments or unimplemented features.", srcLoc->getLine(), srcLoc->getFile());
		}
		
		// Return value data
		std::shared_ptr<void> ret;
		if (returnType != &ffi_type_void) { // Void doesnt need memory
			ret = std::shared_ptr<void>(std::aligned_alloc(returnType->alignment, returnType->size), [](void* ptr){free(ptr);});
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
				if (not std::holds_alternative<std::shared_ptr<Object>>(args.at(i))) {
					throw InterpreterException("Cannot create struct from value argument", srcLoc->getLine(), srcLoc->getFile());
				}
				auto obj = std::get<std::shared_ptr<Object>>(args.at(i));
				// Create struct
				const ffi_type* type = paramTypes.at(i);
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
					throw InterpreterException("Cannot have void as param type", srcLoc->getLine(), srcLoc->getFile());
				case CType::Uint8:
					arguments.push_back(toAny<uint8_t>(value, pType.pointer, altHeap));
					break;
				case CType::Sint8:
					arguments.push_back(toAny<int8_t>(value, pType.pointer, altHeap));
					break;
				case CType::Uint16:
					arguments.push_back(toAny<uint16_t>(value, pType.pointer, altHeap));
					break;
				case CType::Sint16:
					arguments.push_back(toAny<int16_t>(value, pType.pointer, altHeap));
					break;
				case CType::Uint32:
					arguments.push_back(toAny<uint32_t>(value, pType.pointer, altHeap));
					break;
				case CType::Sint32:
					arguments.push_back(toAny<int32_t>(value, pType.pointer, altHeap));
					break;
				case CType::Uint64:
					arguments.push_back(toAny<uint64_t>(value, pType.pointer, altHeap));
					break;
				case CType::Sint64:
					arguments.push_back(toAny<int64_t>(value, pType.pointer, altHeap));
					break;
				case CType::Float:
					arguments.push_back(toAny<float>(value, pType.pointer, altHeap));
					break;
				case CType::Double:
					arguments.push_back(toAny<double>(value, pType.pointer, altHeap));
					break;
				case CType::Uchar:
					arguments.push_back(toAny<unsigned char>(value, pType.pointer, altHeap));
					break;
				case CType::Schar:
					arguments.push_back(toAny<signed char>(value, pType.pointer, altHeap));
					break;
				case CType::Ushort:
					arguments.push_back(toAny<unsigned short>(value, pType.pointer, altHeap));
					break;
				case CType::Sshort:
					arguments.push_back(toAny<short>(value, pType.pointer, altHeap));
					break;
				case CType::Uint:
					arguments.push_back(toAny<unsigned int>(value, pType.pointer, altHeap));
					break;
				case CType::Sint:
					arguments.push_back(toAny<int>(value, pType.pointer, altHeap));
					break;
				case CType::Ulong:
					arguments.push_back(toAny<unsigned long>(value, pType.pointer, altHeap));
					break;
				case CType::Slong:
					arguments.push_back(toAny<long>(value, pType.pointer, altHeap));
					break;
				case CType::Longdouble:
					arguments.push_back(toAny<long double>(value, pType.pointer, altHeap));
					break;
				case CType::Cstring:
					if (pType.pointer) {
						throw InterpreterException("Unimplemented feature", srcLoc->getLine(), srcLoc->getFile());
					} else {
						altHeap.push_back(std::get<std::string>(value));
						arguments.push_back(std::any_cast<std::string&>(altHeap.back()).data());
					}
					break;					
				default:
					throw InterpreterException("Unimplemented arg type", srcLoc->getLine(), srcLoc->getFile());
				}					
			}
		}

		// This should demonstrate what in the world is happening here
		// std::any a = 18;
		// void* b = &std::any_cast<int&>(a);
		// assert(*((int*)b) == 18);
		
		// Make call_args a list of void pointers to the actual values
		for (int i = 0; i < narms; ++i) {
			// nightmare nightmare nightmare 
			// This is quite prone to errors, errors which are likely quite hard to spot
			const Type& pType = func.argTypes.at(i);
			
			switch (pType.type)
			{
			case CType::Void:
				throw InterpreterException("Cannot have void as param type", srcLoc->getLine(), srcLoc->getFile());
			case CType::Uint8:
				call_args.push_back(toVoid<uint8_t>(arguments.at(i), pType.pointer));
				break;
			case CType::Sint8:
				call_args.push_back(toVoid<int8_t>(arguments.at(i), pType.pointer));
				break;
			case CType::Uint16:
				call_args.push_back(toVoid<uint16_t>(arguments.at(i), pType.pointer));
				break;
			case CType::Sint16:
				call_args.push_back(toVoid<int16_t>(arguments.at(i), pType.pointer));
				break;
			case CType::Uint32:
				call_args.push_back(toVoid<uint32_t>(arguments.at(i), pType.pointer));
				break;
			case CType::Sint32:
				call_args.push_back(toVoid<int32_t>(arguments.at(i), pType.pointer));
				break;
			case CType::Uint64:
				call_args.push_back(toVoid<uint64_t>(arguments.at(i), pType.pointer));
				break;
			case CType::Sint64:
				call_args.push_back(toVoid<int64_t>(arguments.at(i), pType.pointer));
				break;
			case CType::Float:
				call_args.push_back(toVoid<float>(arguments.at(i), pType.pointer));
				break;
			case CType::Double:
				call_args.push_back(toVoid<double>(arguments.at(i), pType.pointer));
				break;
			case CType::Uchar:
				call_args.push_back(toVoid<unsigned char>(arguments.at(i), pType.pointer));
				break;
			case CType::Schar:
				call_args.push_back(toVoid<signed char>(arguments.at(i), pType.pointer));
				break;
			case CType::Ushort:
				call_args.push_back(toVoid<unsigned short>(arguments.at(i), pType.pointer));
				break;
			case CType::Sshort:
				call_args.push_back(toVoid<short>(arguments.at(i), pType.pointer));
				break;
			case CType::Uint:
				call_args.push_back(toVoid<unsigned int>(arguments.at(i), pType.pointer));
				break;
			case CType::Sint:
				call_args.push_back(toVoid<int>(arguments.at(i), pType.pointer));
				break;
			case CType::Ulong:
				call_args.push_back(toVoid<unsigned long>(arguments.at(i), pType.pointer));
				break;
			case CType::Slong:
				call_args.push_back(toVoid<long>(arguments.at(i), pType.pointer));
				break;
			case CType::Longdouble:
				call_args.push_back(toVoid<long double>(arguments.at(i), pType.pointer));
				break;				
			case CType::Cstring:
				if (pType.pointer) {
					throw InterpreterException("Unimplemented feature", srcLoc->getLine(), srcLoc->getFile());
				} else {
					call_args.push_back(toVoid<char>(arguments.at(i), true));
				}
				break;
			case CType::Struct:
				call_args.push_back(std::any_cast<void*&>(arguments.at(i)));
				break;
			default:
				throw InterpreterException("Unimplemented arg type", srcLoc->getLine(), srcLoc->getFile());
			}			
		}

		assert(call_args.size() == narms);
		assert(arguments.size() == narms);
		
		// Call the function
		ffi_call(&cif, FFI_FN(func.function), ret.get(), call_args.data());
		
		// Write pointer values back to their Runtime counterparts
		for (int i = 0; i < narms; ++i) {
			const Type& t = func.argTypes.at(i);
			// Check if struct, as they may have pointer members
			if (t.type == CType::Struct)
			{
				if (auto pObj = std::get_if<std::shared_ptr<Object>>(&args.at(i))) {
					updateObject(call_args.at(i), *pObj, t);
				} else {
#if RUNTIME_DEBUG==1
					std::cout << "Value passed to struct argument! New values are not written down! Type: " << static_cast<int>(t.type) << std::endl;
#endif // RUNTIME_DEBUG

				}
			}
			else if (t.pointer or t.type == CType::Cstring)
			{
				if (auto pObj = std::get_if<std::shared_ptr<Object>>(&args.at(i))) {
					// First check if string
					if (t.type == CType::Cstring) {
						pObj->get()->setLast(*reinterpret_cast<char**>(call_args.at(i)));
						continue;
					}
					// If not string
					double val;
					switch (t.type)
					{
					case CType::Uint8:
						val = **reinterpret_cast<uint8_t**>(call_args.at(i));
						break;
					case CType::Sint8:
						val = **reinterpret_cast<int8_t**>(call_args.at(i));
						break;
					case CType::Uint16:
						val = **reinterpret_cast<uint16_t**>(call_args.at(i));
						break;
					case CType::Sint16:
						val = **reinterpret_cast<int16_t**>(call_args.at(i));
						break;
					case CType::Uint32:
						val = **reinterpret_cast<uint32_t**>(call_args.at(i));
						break;
					case CType::Sint32:
						val = **reinterpret_cast<int32_t**>(call_args.at(i));
						break;
					case CType::Uint64:
						val = **reinterpret_cast<uint64_t**>(call_args.at(i));
						break;
					case CType::Sint64:
						val = **reinterpret_cast<int64_t**>(call_args.at(i));
						break;
					case CType::Float:
						val = **reinterpret_cast<float**>(call_args.at(i));
						break;
					case CType::Double:
						val = **reinterpret_cast<double**>(call_args.at(i));
						break;
					case CType::Uchar:
						val = **reinterpret_cast<unsigned char**>(call_args.at(i));
						break;
					case CType::Schar:
						val = **reinterpret_cast<signed char**>(call_args.at(i));
						break;
					case CType::Ushort:
						val = **reinterpret_cast<unsigned short**>(call_args.at(i));
						break;
					case CType::Sshort:
						val = **reinterpret_cast<short**>(call_args.at(i));
						break;
					case CType::Uint:
						val = **reinterpret_cast<unsigned int**>(call_args.at(i));
						break;
					case CType::Sint:
						val = **reinterpret_cast<int**>(call_args.at(i));
						break;
					case CType::Ulong:
						val = **reinterpret_cast<unsigned long**>(call_args.at(i));
						break;
					case CType::Slong:
						val = **reinterpret_cast<long**>(call_args.at(i));
						break;
					case CType::Longdouble:
						val = **reinterpret_cast<long double**>(call_args.at(i));
						break;
					default:
						throw InterpreterException("Unimplemented element type", srcLoc->getLine(), srcLoc->getFile());
					}
					pObj->get()->setLast(val);
				} else {
#if RUNTIME_DEBUG==1
					std::cout << "Value passed to pointer argument! New values are not written down! Type: " << static_cast<int>(t.type) << std::endl;
#endif // RUNTIME_DEBUG
				}
			}
			// Otherwise no need to update any values
		}
		
		// Return value
		if (returnType->type == FFI_TYPE_STRUCT) {
			// Construct Runtime object based on struct in memory
			return objectFromStruct(ret.get(), func.retType.value());
		} else {
			switch (func.retType.value().type)
			{
			case CType::Void:
				return True; // Return True to indicate success
			case CType::Uint8:
				return static_cast<double>(*reinterpret_cast<uint8_t*>(ret.get()));
			case CType::Sint8:
				return static_cast<double>(*reinterpret_cast<int8_t*>(ret.get()));
			case CType::Uint16:
				return static_cast<double>(*reinterpret_cast<uint16_t*>(ret.get()));
			case CType::Sint16:
				return static_cast<double>(*reinterpret_cast<int16_t*>(ret.get()));
			case CType::Uint32:
				return static_cast<double>(*reinterpret_cast<uint32_t*>(ret.get()));
			case CType::Sint32:
				return static_cast<double>(*reinterpret_cast<int32_t*>(ret.get()));
			case CType::Uint64:
				return static_cast<double>(*reinterpret_cast<uint64_t*>(ret.get()));
			case CType::Sint64:
				return static_cast<double>(*reinterpret_cast<int64_t*>(ret.get()));
			case CType::Float:
				return static_cast<double>(*reinterpret_cast<float*>(ret.get()));
			case CType::Double:
				return static_cast<double>(*reinterpret_cast<double*>(ret.get()));
			case CType::Uchar:
				return static_cast<double>(*reinterpret_cast<unsigned char*>(ret.get()));
			case CType::Schar:
				return static_cast<double>(*reinterpret_cast<signed char*>(ret.get()));
			case CType::Ushort:
				return static_cast<double>(*reinterpret_cast<unsigned short*>(ret.get()));
			case CType::Sshort:
				return static_cast<double>(*reinterpret_cast<short*>(ret.get()));
			case CType::Uint:
				return static_cast<double>(*reinterpret_cast<unsigned int*>(ret.get()));
			case CType::Sint:
				return static_cast<double>(*reinterpret_cast<int*>(ret.get()));
			case CType::Ulong:
				return static_cast<double>(*reinterpret_cast<unsigned long*>(ret.get()));
			case CType::Slong:
				return static_cast<double>(*reinterpret_cast<long*>(ret.get()));
			case CType::Longdouble:
				return static_cast<double>(*reinterpret_cast<long double*>(ret.get()));
			default:
				throw InterpreterException("Unimplemented return type", srcLoc->getLine(), srcLoc->getFile());
			}
		}
	}
}
