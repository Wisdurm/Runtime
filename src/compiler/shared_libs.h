#pragma once
// Runtime
#include "object.h"
// C++
#include <ffi.h>
#include <memory>
#include <variant>
#include <vector>
#include <cstring>
#include <experimental/memory>
#include <any>
// C
#include <cstdlib>
#include <ffi.h>

namespace rt
{
	// Forward declarations
	class SymbolTable;
	class ArgState;

	/// <summary>
	/// Custom type which is identical to pointer type, but which interpreter has custom behaviour for
	/// </summary>
	inline ffi_type ffi_type_cstring = ffi_type_pointer;
	// Custom pointer types
	inline ffi_type ffi_type_puint8 = ffi_type_pointer;
	inline ffi_type ffi_type_psint8 = ffi_type_pointer;
	inline ffi_type ffi_type_puint16 = ffi_type_pointer;
	inline ffi_type ffi_type_psint16 = ffi_type_pointer;
	inline ffi_type ffi_type_puint32 = ffi_type_pointer;
	inline ffi_type ffi_type_psint32 = ffi_type_pointer;
	inline ffi_type ffi_type_puint64 = ffi_type_pointer;
	inline ffi_type ffi_type_psint64 = ffi_type_pointer;
	inline ffi_type ffi_type_pfloat = ffi_type_pointer;
	inline ffi_type ffi_type_pdouble = ffi_type_pointer;
	inline ffi_type ffi_type_puchar = ffi_type_pointer;
	inline ffi_type ffi_type_pschar = ffi_type_pointer;
	inline ffi_type ffi_type_pushort = ffi_type_pointer;
	inline ffi_type ffi_type_psshort = ffi_type_pointer;
	inline ffi_type ffi_type_puint = ffi_type_pointer;
	inline ffi_type ffi_type_psint = ffi_type_pointer;
	inline ffi_type ffi_type_pulong = ffi_type_pointer;
	inline ffi_type ffi_type_pslong = ffi_type_pointer;
	inline ffi_type ffi_type_plongdouble = ffi_type_pointer;
	// TODO: Complex I guess?
	
	/// <summary>
	/// Container for a function which has been loaded from a shared library
	/// </summary>
	struct LibFunc
	{
		/// <summary>
		/// Pointer to the function
		/// </summary>
		void* function;
		/// <summary>
		/// Whether or not this function has all of its parameters and return types specified
		/// </summary>
		bool initialized;
		/// <summary>
		/// Return type of the function
		/// </summary>
		std::variant<std::experimental::observer_ptr<ffi_type>, std::shared_ptr<ffi_type>> retType;
		/// <summary>
		/// Argument types
		/// </summary>
		std::vector<std::variant<std::experimental::observer_ptr<ffi_type>, std::shared_ptr<ffi_type>>> argTypes;
		/// <summary>
		/// Smart pointer array; stores values that need to exist for this objects lifetime
		/// </summary>
		std::vector<std::any> altHeap;
	};

	/// <summary>
	/// Loads a shared library
	/// </summary>
	void loadSharedLibrary(const char* fileName, SymbolTable* symtab);
	/// <summary>
	/// Unloads all shared libraries
	/// </summary>
	void cleanLibraries();
        /// <summary>
	/// Calls a shared library function
	/// </summary>
	objectOrValue callShared(const std::vector<objectOrValue>& args, const LibFunc& func, SymbolTable* symtab, ArgState& argState);
}
