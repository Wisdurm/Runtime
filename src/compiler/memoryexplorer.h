#include <cstdint>
#include <sys/types.h>
#include "ffi.h"
#include <tuple>

// Class to provide a convenient and (hopefully) safe wrapper for a section of memory
// to be accessed as a ffi_type mandated struct
class MemoryExplorer {
public:
MemoryExplorer(void* mem, ffi_type* definition) : memory(mem), definition(definition) {};
	// Accesses the nth member of the struct
	inline std::pair<void*, ffi_type*> operator[] (int memberIndex)
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
