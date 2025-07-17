// Runtime.cpp : Defines the entry point for the application.

#include "Runtime.h"
#include "compiler/tokenizer.h"

#include "../tests/tokenizer_tests.h"
#include "../tests/parser_tests.h"

int main(int argc, char* argv[])
{
	// Google roblox
	rt::test_tokenizer_basics();
	rt::test_parser_basics();
	return 0;
}
