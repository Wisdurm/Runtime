#pragma once
// Runtime
#include "shared_libs.h"
#include "exceptions.h"
// C++
#include <tuple>
// C
#include <cstdint>
#include <readline/rlstdc.h>
#include <strings.h>
#include <sys/types.h>
#include "ffi.h"

// Class to provide a convenient and (hopefully) safe wrapper for a section of memory
// to be accessed as a rt::Type mandated struct
class MemoryExplorer {
public:
	MemoryExplorer(void* mem, const rt::Type& definition) : memory(mem), definition(definition) {};
	// Accesses the nth member of the struct
	std::pair<void*, const rt::Type&> operator[] (int memberIndex)
	{
		uint8_t* p = static_cast<uint8_t*>(memory); // Move a byte at a time
		for (int i = 0, totSize = 0; i < definition.members.size(); i++) {
			// Loop through member types
			const auto t = definition.members.at(i).get();
			const int pos = totSize + (totSize%t->alignment); // Align position to next block of preferred alignment
			if (i == memberIndex) {
				return { p+pos, definition.members.at(i)};
			}
			totSize += t->size + (totSize%t->alignment);
		}
		throw InterpreterException("Unable to get desired type from struct", 0, "Unknown");
	}
private:
	// Pointer to the section of memory that holds the specified struct
	void* memory;
	// The ffi_type that defines what the section of memory holds
	const rt::Type& definition;
};
