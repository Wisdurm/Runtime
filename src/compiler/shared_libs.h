#pragma once
// Runtime
#include "object.h"
#include "interpreter.h"
// C++
#include <algorithm>
#include <deque>
#include <ffi.h>
#include <memory>
#include <optional>
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
		// TODO: Replace shared_ptr with unique_ptr when brave enough
		
		// The actual type
		const CType type;
		// Whether or not pointer
		const bool pointer;
		// Members (If struct)
		std::vector<Type> members;
		// Libffi type: either observer pointer to existing one, or shared pointer owning struct type
		std::variant<std::experimental::observer_ptr<ffi_type>, std::shared_ptr<ffi_type>> ffiType;
		// Element array used by ffi_type
		std::vector<ffi_type*> elements;

		// Returns the pointer either owned or observed by the type
		ffi_type* get() const
		{
			if (auto pp = std::get_if<std::experimental::observer_ptr<ffi_type>>(&ffiType)) {
				return pp->get();
			} else {
				return std::get<std::shared_ptr<ffi_type>>(ffiType).get();
			}
		}
		
		// Constructor without members
		Type(CType type, bool pointer)
			: type(type)
			, pointer(pointer)
		{

			ffiType = makeFfiType();
		}

		// Constructor with members
		Type(CType type, bool pointer, std::vector<Type>& members)
			: type(type)
			, pointer(pointer)
			, members(std::move(members))
		{
			ffiType = makeFfiType();
		}

		// Never copy just because
		Type (const Type&) = delete;
		Type& operator= (const Type&) = delete;
		// Move constructor; Should be safe, assuming Type.ffiType.elements still points to Type.elements
		Type (Type&& other)
			: type(other.type)
			, pointer(other.pointer)
			, ffiType(other.ffiType)
			, members(std::move(other.members))
			, elements(std::move(other.elements)) // Not sure if move is necessary but must remain stable
		{
			// Idk?
		};
		// Dont need so delete juuuuusttt in case
		Type& operator= (const Type&&) = delete;
	private:
		/// Returns ffi_type representing Type
		[[nodiscard]] std::variant<std::experimental::observer_ptr<ffi_type>, std::shared_ptr<ffi_type>> makeFfiType()
		{
			if (type == CType::Struct) {
				const int size = members.size();
				// Struct
				elements.reserve(size);
				// Member types
				for (int i = 0; i < size; ++i) {					
					elements.push_back(members.at(i).get());
				}
				elements.push_back(NULL); // Last one (NULL terminated array)
				assert(elements.size() == size);
				return std::shared_ptr<ffi_type>(new ffi_type(
								  0, // size // These will be calculated by libffi later
								  0, // align (init 0)
								  FFI_TYPE_STRUCT, // type
								  elements.data() // elements
								  ));
			} else {
				if (pointer) {
					return std::experimental::make_observer<ffi_type>(&ffi_type_pointer);
				} else {
					return std::experimental::make_observer<ffi_type>(typeMap.at(type));
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
