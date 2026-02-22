// Runtime
#include "shared_libs.h"
#include "interpreter.h"
#include "exceptions.h"
#include "Stlib/StandardFiles.h"
#include "symbol_table.h"
// C++
#include <algorithm>
#include <any>
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
	static const std::array<ffi_type*, 20> ffiTypes = {
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

	[[nodiscard]] objectOrValue callShared(const std::vector<objectOrValue>& args, const LibFunc& func, SymbolTable* symtab, ArgState& argState)
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
