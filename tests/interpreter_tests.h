#pragma once

#include "../src/compiler/tokenizer.h"
#include "../src/compiler/parser.h"
#include "../src/compiler/interpreter.h"
// C++
#include <vector>
#include <cassert>
#include <iostream>

namespace rt
{
	// not sure how this file has access to L but I'll look into it TODO

	/// <summary>
	/// Test basic function of the tokenizer
	/// </summary>
	void test_interpreter_basics()
	{
		std::cout << "Testing interpreter basics..." << std::endl;

		// Test basic string printing
		std::vector<std::string> test1 = {"Hello world"};
		ast::Expression* r1 = new ast::Call(L, "Print", { new ast::Literal(L, ast::value("Hello world")) });
		assert(interpretAndReturn(r1) == test1.data());
		delete r1;

		std::cout << "Passed interpreter basics!" << std::endl;
	}

}