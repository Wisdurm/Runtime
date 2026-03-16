#include <cstddef>
#include <cstdint>
#include <readline/rlstdc.h>
#include <strings.h>
#include <sys/types.h>
#include "ffi.h"
#include "interpreter.h"
#include <any>
#include <deque>
#include <vector>
#include <array>
#include <algorithm>

// Class to provide a convenient and (hopefully) safe wrapper for a section of memory
// to be accessed as a ffi_type mandated struct
class MemoryExplorer {
public:
MemoryExplorer(void* mem, ffi_type* definition) : memory(mem), definition(definition) {};
	// Accesses the nth member of the struct
	std::pair<void*, ffi_type*> operator[] (int memberIndex)
	{
		uint8_t* p = static_cast<uint8_t*>(memory); // Move a byte at a time
		for (int i = 0, totSize = 0; definition->elements[i] != NULL; ++i) {
			// Loop through member types
			const auto t = definition->elements[i];
			const int pos = totSize + (totSize%t->alignment); // Align position to next block of preferred alignment
			if (i == memberIndex) {
				return { p+pos, t};
			}

			totSize += t->size + (totSize%t->alignment);
		}
		return { nullptr, nullptr};
	}
private:
	// Pointer to the section of memory that holds the specified struct
	void* memory;
	// The ffi_type that defines what the section of memory holds
	ffi_type* definition;
};

/// Real libffi ffi_types
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

// Wrapper around the params provided to libffi, in order to allow
// custom ffi_types
class ParamWrapper {
public:
	// Constructor
	ParamWrapper(std::size_t narms)
	{
		params.reserve(narms);
		paramsAbstract.reserve(narms);
	}

	// Add real or custom ffi type
	void add(ffi_type* type)
	{
		paramsAbstract.push_back(type);
		// If struct, check members
		if (type->type == FFI_TYPE_STRUCT) {
			// Recursively copy struct and remove custom types
			auto copy = [this](ffi_type* type, auto&& copy) -> ffi_type* {
				if (type->type == FFI_TYPE_STRUCT) {
					// Create new struct
					auto strc = rt::altStore<ffi_type>(new ffi_type (
								       0, // size // These will be copied later
								       0, // align (init 0)
								       FFI_TYPE_STRUCT, // type
								       nullptr // elements
								       ), altHeap);
					// Copy values
					int len = 0;
					for (; type->elements[len] != NULL; len++) {}
					strc->elements = rt::altStore<ffi_type*>(new ffi_type*[len], altHeap);
					for (int i = 0; type->elements[i] != NULL; i++) {
						strc->elements[i] = copy(type->elements[i], copy);
					};
					return strc;
				} else {
					// If custom type, replace
					if (std::find(ffiTypes.begin(), ffiTypes.end(), type) == ffiTypes.end()) {
						return &ffi_type_pointer;
					} else {
						return type;
					}
				}
			};			
			params.push_back(copy(type, copy));
		} else {
			// If not real ffi_type, replace with pointer
			if (std::find(ffiTypes.begin(), ffiTypes.end(), type))
				params.push_back(type);
			else
				params.push_back(&ffi_type_pointer);
		}
	}

	// ffi_prep_cif only calculates values for the real params, so we need
	// to copy these values onto paramsAbstract
	void updateAbstract()
	{
		for (int i = 0; i < params.size(); i++) {
			// Recursively copy struct and remove custom types
			auto update = [this](ffi_type* type, ffi_type* typeA, auto&& update) -> void {
				// Copy struct values
				if (type->type == FFI_TYPE_STRUCT) {
					typeA->alignment = type->alignment;
					typeA->size = type->size;
					for (int i = 0; type->elements[i] != NULL; i++) {
						update(type->elements[i], typeA->elements[i], update);
					};
				}
			};
			update(params.at(i), paramsAbstract.at(i), update);
		}
	}

	// Returns the real ffi_type params stored
	ffi_type** realParams()
	{
		return params.data();
	}

	// Returns a real or custom ffi_type at index
	ffi_type*& at(int index)
	{
		return paramsAbstract.at(index);
	}

	// Returns whether or not the type at index is a custom or real one
	bool is_custom_at(int index)
	{
		return params.at(index) != paramsAbstract.at(index);
	}
private:
	// Array of pointers to real ffi_types
	std::vector<ffi_type*> params;
	// Array of pointes to ffi_types, real or custom
	std::vector<ffi_type*> paramsAbstract;
	// Store rubbish
	std::deque<std::any> altHeap;
};
