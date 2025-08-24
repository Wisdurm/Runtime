// Runtime.cpp : Defines the entry point for the application.

#include "Runtime.h"
#include "compiler/tokenizer.h"
#include "compiler/parser.h"
#include "compiler/interpreter.h"

// C++
#include <iostream>

#define RUNTIME_VERSION "v0.8.0"

int main(int argc, char* argv[])
{
	std::cout << "\033[33m" << "  _____             _   _                \n |  __ \\           | | (_)               \n | |__) |   _ _ __ | |_ _ _ __ ___   ___ \n |  _  / | | | '_ \\| __| | '_ ` _ \\ / _ \\\n | | \\ \\ |_| | | | | |_| | | | | | |  __/\n |_|  \\_\\__,_|_| |_|\\__|_|_| |_| |_|\\___|\n";
	std::cout << "\033[37m" << std::endl << "Running Runtime " << RUNTIME_VERSION << std::endl;
	// Interpreter
	rt::liveIntrepretSetup();
	while (true)
	{
		std::cout << "\033[33m" << ">> " << "\033[37m";
		std::string input;
		std::getline(std::cin,input);
		rt::liveIntrepret(rt::parse(rt::tokenize(input.c_str()), false));
	}
	return 0;
}