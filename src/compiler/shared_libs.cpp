// Runtime
#include "shared_libs.h"
#include "interpreter.h"
#include "exceptions.h"
#include "Stlib/StandardFiles.h"
#include "object.h"
#include "symbol_table.h"
#include "memoryexplorer.h"
// C++
#include <algorithm>
#include <any>
#include <deque>
#include <ffi.h>
#include <list>
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

namespace rt
{
	/// <summary>
	/// Stores all currently loaded shared libraries
	/// </summary>
	static std::vector<void*> libraries;
	/// <summary>
	/// ffi_types
	/// </summary>
	static constexpr std::array<ffi_type*, 20> ffiTypes = {
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
			symtab->updateSymbol(sym, LibFunc{
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
				     SymbolTable* symtab, ArgState& argState, std::deque<std::any>& altHeap)
	{
		// Here we assume we already have all the memory we need allocated;
		// this function does not allocate any memory, we are passed the structMem
		// argument, and we are to believe that thanks to libffi's genius, everything
		// difficult is already dealt with

		// TODO: Packed support, as well as look into the libffi way of doing this
		// Get members of arg
		auto members = obj->getMembers(); // TODO: error handling
		MemoryExplorer expl = MemoryExplorer(structMem, &type);
		// Assign members
		for (int i = 0; type.elements[i] != NULL; ++i) {
			// Loop through member types
			const auto [memory, t] = expl[i];
			if (t->type == FFI_TYPE_STRUCT) { // Struct
				auto member = std::get<std::shared_ptr<Object>>(members.at(i)); // TODO: Error handling
				structFromObject(memory, member, *t, symtab, argState, altHeap);
			} else {
				const auto value = evaluate(members.at(i), symtab, argState);
				if (auto str = std::get_if<std::string>(&value)) {
					// TODO: Maybe should be stored somewhere else? Idk
					altHeap.push_back(*str);
					*reinterpret_cast<char**>(memory) = std::any_cast<std::string&>(altHeap.back()).data();
				} else {
					double val = std::get<double>(value);
					if (t == &ffi_type_sint)  // TODO TE RES
						*reinterpret_cast<int*>(memory) = static_cast<int>(val);
					else if (t == &ffi_type_uchar) 
						*reinterpret_cast<unsigned char*>(memory) = static_cast<unsigned char>(val);
					else if (t == &ffi_type_float) 
						*reinterpret_cast<float*>(memory) = static_cast<float>(val);
					// Pointers
					else if (t == &ffi_type_pfloat)
						*reinterpret_cast<float**>(memory) = altAlloc(static_cast<float>(val), altHeap);
					else throw InterpreterException("Unimplemented element type", 0, "Unknown");
				}
			}
		}
	}

	/// <summary>
	/// Creates a Runtime object from a struct in memory
	/// </summary>
	static std::shared_ptr<Object> objectFromStruct(void* strc, ffi_type structType)
	{
		// TODO: Packed support, as well as look into the libffi way of doing this
		auto obj = std::make_shared<Object>();
		MemoryExplorer expl = MemoryExplorer(strc, &structType);
		for (int i = 0; structType.elements[i] != NULL; ++i) {
			// Loop through member types
			const auto [memory, t] = expl[i];
			// Get value
			std::variant<double, std::string> value;
			if (t == &ffi_type_sint)  // TODO TE RES
				value = static_cast<double>(*reinterpret_cast<int*>(memory));
			else if (t == &ffi_type_float)
				value = static_cast<double>(*reinterpret_cast<float*>(memory));
			else if (t->type == FFI_TYPE_STRUCT) {
				obj->addMember(objectFromStruct(memory, *t));
			}
			else throw InterpreterException("Unimplemented element type", 0, "Unknown");
			// Add value
			obj->addMember(value);
		}
		return obj;
	}

	// Sets the values of a Runtime object based on pointers within a struct
	static void updateObject(ffi_type strct, ffi_type strctCpy, std::shared_ptr<Object> obj, void* callArg)
	{
		// Loop through all the members and check if they are pointers
		MemoryExplorer expl = MemoryExplorer(callArg, &strctCpy);
		for (int i = 0; strct.elements[i] != NULL; i++) {
			// Loop through member types
			const auto [memory, t] = expl[i];
			auto member = obj->getMembers()[i];
			if (strct.elements[i] != strctCpy.elements[i]) { // Check if custom type, as they're the only pointers
				// First check if string
				if (strctCpy.elements[i] == &ffi_type_cstring) {
					char* str = *reinterpret_cast<char**>(memory);
					if (auto op = std::get_if<std::shared_ptr<Object>>(&member)) {
						(*op)->setLast(str);
					} else {
						std::get<std::variant<double, std::string>>(member) = str;
					}
					continue;
				}
				// If not string
				double val;
				if (strctCpy.elements[i] == &ffi_type_psint)
					val = **reinterpret_cast<int**>(memory);
				if (strctCpy.elements[i] == &ffi_type_pfloat)
					val = **reinterpret_cast<float**>(memory);
				// TODO: The rest
				if (auto op = std::get_if<std::shared_ptr<Object>>(&member)) {
					(*op)->setLast(val);
				} else {
					std::get<std::variant<double, std::string>>(member) = val;
				}
			} else if (strct.elements[i]->type == FFI_TYPE_STRUCT) {
				updateObject(*strct.elements[i], *t, std::get<std::shared_ptr<Object>>(member), memory); // TODO: Error handling for get<>
			}
		}
	}

	// Copy ffi types from one array to another
	static void copyTypeArray(ffi_type**& srcArray, ffi_type**& destArray, int len, std::deque<std::any>& altHeap)
	{
		// Not really sure if this works, TODO: Tests
		for (int i = 0; i < len; i++) {
			const auto copy = [&altHeap](ffi_type*& src, ffi_type*& dest, auto&& copy) -> void {
				if (src->type == FFI_TYPE_STRUCT) {
					// I cannot believe this
					dest = altStore<ffi_type>(new ffi_type, altHeap);
					int len = 0;
					for (; src->elements[len] != NULL; len++) {}
					dest->elements = altStore<ffi_type*>(new ffi_type*[len], altHeap);
					for (int i = 0; src->elements[i] != NULL; i++) {
						copy(src->elements[i], dest->elements[i], copy);
					}
				} else {
					// Just copy the pointer, trivial
					dest = src;
				}
			};
			copy(srcArray[i], destArray[i], copy);
		}
	}

	[[nodiscard]] objectOrValue callShared(const std::vector<objectOrValue>& args, const LibFunc& func, SymbolTable* symtab, ArgState& argState)
	{
		// TODO: There are probably about 1000 memory leaks, memory mismagement and whatever errors in this code.
		// I'm not even sure where to begin honestly.
		
		// TODO: Windows
		if (not func.initialized)
			throw InterpreterException("Shared function is not yet bound", 0, "Unknown");
		// Stores smart pointers which will be deallocated at the end
		// The pointers store values that need to be stored in this scope
		// but cant be smart pointers by themselves, since they're
		// created inline, and need to pass pointers to libffi.
		// Understand?
		std::deque<std::any> altHeap;
		
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
		// We need to have a copy of params, since params may have "custom" types,
		// which arent actually a part of libffi, but I added for "convenience".
		// These custom types cannot be used in ffi_call, since it will just not work
		// as intended.
		ffi_type** paramsCopy = new ffi_type*[narms];
		// Deep copy params
		copyTypeArray(params, paramsCopy, narms, altHeap);
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
			ret = new char[returnType->size]; // TODO: alligned alloc?? idk...
		}
		// Arguments
		std::vector<std::any> arguments;
		// List of generic pointers to the values actually used to call the function
		std::unique_ptr<void* []> call_args; // Generic pointers to args
		call_args = std::make_unique<void* []>(narms); // Create list to arg pointers
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
				structFromObject(structMem, obj, *type, symtab, argState, altHeap);
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
				// Pointer types
				else if (type == &ffi_type_cstring) {
					// Add to alt heap so object lifetime survives function call
					altHeap.push_back(std::get<std::string>(value));
					arguments.push_back(std::any_cast<std::string&>(altHeap.back()).data());
				}
				else if (type == &ffi_type_psint)
					arguments.push_back(altAlloc<int>(getNumericalValue(value), altHeap));
				else if (type == &ffi_type_pfloat)
					arguments.push_back(altAlloc<float>(getNumericalValue(value), altHeap));
				// TODO: The rest
				else throw InterpreterException("Unimplemented arg type", 0, "Unknown");
				// TODO: Other types
				/*}}}*/
			}
		}
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
			else if (arguments.at(i).type() == typeid(void*)) // Struct
				call_args[i] = std::any_cast<void*>(arguments.at(i));
			// Pointers
			else if (arguments.at(i).type() == typeid(char*)) // C string
				call_args[i] = &std::any_cast<char*&>(arguments.at(i));		
			else if (arguments.at(i).type() == typeid(int*))
				call_args[i] = &std::any_cast<int*&>(arguments.at(i));
			else if (arguments.at(i).type() == typeid(float*))
				call_args[i] = &std::any_cast<float*&>(arguments.at(i));
			// TODO: The rest
			else throw InterpreterException("Unimplemented arg pointer", 0, "Unknown");
			/*}}}*/
			}
		}
		// Turn custom ffi_types into pointers
		// ffi_call will not work properly if this is not done
		for (int i = 0; i < narms; ++i) {
			// TODO: I don't think this works at all
			if (params[i]->type == FFI_TYPE_STRUCT) {
				auto restore = [](ffi_type* strct, auto&& restore) -> void {
					for (int i = 0; strct->elements[i] != NULL; i++) {
						if (strct->elements[i]->type == FFI_TYPE_STRUCT) {
							restore(strct->elements[i], restore);
						} else if (std::find(ffiTypes.begin(), ffiTypes.end(), strct->elements[i]) == ffiTypes.end()) {
							strct->elements[i] = &ffi_type_pointer;
						}
					};
				};
				restore(params[i], restore);
			}
			else if (std::find(ffiTypes.begin(), ffiTypes.end(), params[i]) == ffiTypes.end()) {
				params[i] = &ffi_type_pointer;	
			}
		}
		// Call
		ffi_call(&cif, FFI_FN(func.function), ret, call_args.get());
		// Write pointer values back to their Runtime counterparts
		for (int i = 0; i < narms; ++i) {
			if (params[i] != paramsCopy[i]) // Check if custom type, as they're the only pointers
			{
				if (auto pObj = std::get_if<std::shared_ptr<Object>>(&args.at(i))) { // TODO: if optimization // ????
					// First check if string
					if (paramsCopy[i] == &ffi_type_cstring) {
						pObj->get()->setLast(*reinterpret_cast<char**>(call_args[i]));
						continue;
					}
					// If not string
					double val;
					if (paramsCopy[i] == &ffi_type_psint)
						val = **reinterpret_cast<int**>(call_args[i]);
					if (paramsCopy[i] == &ffi_type_pfloat)
						val = **reinterpret_cast<float**>(call_args[i]);
					// TODO: The rest
					pObj->get()->setLast(val);
				} else {
					std::cout << "Value passed to pointer argument! New values are not written down! Type: " <<  paramsCopy[i]->type << std::endl;
				}
			}
			if (params[i]->type == FFI_TYPE_STRUCT) // Check if struct, as they may have pointer members
			{
				if (auto pObj = std::get_if<std::shared_ptr<Object>>(&args.at(i))) { // TODO: if optimization // ????
					updateObject(*params[i], *paramsCopy[i], *pObj, call_args[i]);
				} else {
					std::cout << "Value passed to struct argument! New values are not written down! Type: " <<  paramsCopy[i]->type << std::endl;
				}
			}
		}
		// Finished with params
		delete[] params;
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
