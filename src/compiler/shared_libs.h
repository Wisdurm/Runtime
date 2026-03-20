#pragma once
// Runtime
#include "object.h"
#include "interpreter.h"
// C++
#include <deque>
#include <ffi.h>
#include <memory>
#include <optional>
#include <type_traits>
#include <unordered_map>
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

	enum class CType
	{
		Void,
		Uint8, // Unsigned int8
		Sint8, // Signed int8
		Uint16,
		Sint16,
		Uint32,
		Sint32,
		Uint64,
		Sint64,
		Float,
		Double,
		Uchar,
		Schar,
		Ushort,
		Sshort,
		Uint,
		Sint,
		Ulong,
		Slong,
		Longdouble,
		Complexfloat,
		Complexdouble,
		Complexlongdouble,
		Cstring, // C string; pointer with custom behaviour
		Struct
	};

	// Map ctype to ffi_type. Optimize away later
	static const std::unordered_map<CType, ffi_type*> typeMap = {
		
	};
	
	struct Type
	{
		// The actual type
		const CType type;
		// Whether or not pointer
		const bool pointer;
		// Members (If struct)
		const std::deque<Type> members;
		// Libffi type
		std::experimental::observer_ptr<ffi_type> ffiType;
		// Memory; owns all of the memory dynamically allocated via containing
		// smart pointers. Auto de-allocated at the end
		std::deque<std::any> altHeap;

		// Constructor without members
		Type(CType type, bool pointer)
			: type(type)
			, pointer(pointer)
		{
			// This class probably breaks like 100000 established rules of C++
			// but I don't really know how I should be doing this
			ffiType = std::experimental::make_observer<ffi_type>(makeFfiType(*this, altHeap));
		}

		// Constructor with members
		Type(CType type, bool pointer, std::deque<Type>& m)
			: type(type)
			, pointer(pointer)
			, members(std::move(m))
		{
			// This class probably breaks like 100000 established rules of C++
			// but I don't really know how I should be doing this
			ffiType = std::experimental::make_observer<ffi_type>(makeFfiType(*this, altHeap));
		}

		// DONT COPY EVER PLEASE MY SMALL BRAIN CAN'T KEEP UP
		Type (const Type&) = delete;
		Type& operator= (const Type&) = delete;
		// Move constructor; I can't bother anymore, just move it
		Type (const Type&& other)
			: type(other.type)
			, pointer(other.pointer)
			, ffiType(other.ffiType)
			, altHeap(std::move(other.altHeap))
		{
			// I guess this is ok? Idk...
			std::cout << "MOVE CONSTUCTOR CALLED!";
		}
		// Dont need so delete juuuuusttt in case
		Type& operator= (const Type&&) = delete;
	private:
		/// Creates struct ffi_type from Type
		[[nodiscard]] ffi_type* makeFfiType(const Type& type, std::deque<std::any>& altHeap)
		{
			if (type.type == CType::Struct) {
				const int size = type.members.size();
				// Struct
				ffi_type** e = altStore<ffi_type*>(new ffi_type*[size + 1], altHeap);
				// Member types
				for (int i = 0; i < size; ++i) {					
					e[i] = makeFfiType(type.members.at(i), altHeap);
				}
				e[size] = NULL; // Last one (NULL terminated array)
				return altStore<ffi_type>(new ffi_type(
								  0, // size // These will be calculated by libffi later
								  0, // align (init 0)
								  FFI_TYPE_STRUCT, // type
								  e // elements
								  ), altHeap);
			} else {
				if (type.pointer) {
					return &ffi_type_pointer;
				} else {
					return typeMap.at(type.type);
				}
			}
		}
	};
	
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
		std::optional<Type> retType;
		/// <summary>
		/// Argument types
		/// </summary>
		std::deque<Type> argTypes;
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
