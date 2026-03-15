#include <cstddef>
#include <cstdint>
#include <readline/rlstdc.h>
#include <strings.h>
#include <sys/types.h>
#include "ffi.h"
#include <tuple>
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
	void push_back(ffi_type* type)
	{
		paramsAbstract.push_back(type);
		// If struct, check members
		if (type->type == FFI_TYPE_STRUCT) {
			// Recursively check struct for custom types
			auto clean = [](ffi_type* strct, auto&& clean) -> void {
				for (int i = 0; strct->elements[i] != NULL; i++) {
					if (strct->elements[i]->type == FFI_TYPE_STRUCT) {
						clean(strct->elements[i], clean);
					} else if (
						std::find(
							ffiTypes.begin(), ffiTypes.end(),
							strct->elements[i]) == ffiTypes.end()) {
						// Custom type; replace
						strct->elements[i] = &ffi_type_pointer;
					}
				};
			};
			clean(type, clean);	
		} else {
			// If not real ffi_type, replace with pointer
			if (std::find(ffiTypes.begin(), ffiTypes.end(), type))
				params.push_back(type);
			else
				params.push_back(&ffi_type_pointer);
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
};
