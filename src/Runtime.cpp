// Runtime.cpp : Defines the entry point for the application.

#include "Runtime.h"
#include "compiler/tokenizer.h"

#include "../tests/tokenizer_tests.h"
#include "../tests/parser_tests.h"

// C++
#include <iostream>

void test();

int main(int argc, char* argv[])
{
	// Google roblox
	test();
	return 0;
}

/// <summary>
/// Runs all tests
/// </summary>
void test()
{
	std::cout << "Running tests..." << std::endl;
	rt::test_tokenizer_basics();
	rt::test_parser_basics();
	std::cout << "All tests passed!" << std::endl;
}