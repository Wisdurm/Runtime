#pragma once

#include "../src/compiler/tokenizer.h"
// C++
#include <cassert>
#include <vector>

namespace rt
{
	/// <summary>
	/// Test basic function of the tokenizer
	/// </summary>
	void test_tokenizer_basics()
	{
		// Test comments
		char test1[] = "Import(StandardLibrary)\n# I can eat glass and it doesn't hurt me \nObject(Main, Zero)\n";
		std::vector<Token> r1 = { Token("Import(StandardLibrary)", "Object(Main, Zero)")};
		assert(*tokenize(test1) == r1);
	}
}