// Runtime
#include "Runtime.h"
#include "compiler/tokenizer.h"
#include "compiler/parser.h"
#include "compiler/interpreter.h"
// C++
#include <iostream>
#ifdef _WIN32
// Terminal coloring for windows
	
#include <windows.h>
#define YELLOW_TEXT SetConsoleTextAttribute(hConsole, 6);
#define WHITE_TEXT SetConsoleTextAttribute(hConsole, 7);

#else

// Unix-like coloring
#define YELLOW_TEXT std::cout << "\033[33m";
#define WHITE_TEXT std::cout << "\033[37m";

#endif // _WIN32


int main(int argc, char* argv[])
{
#ifdef _WIN32
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE); 
#endif // _WIN32
	YELLOW_TEXT
	std::cout << "  _____             _   _                \n |  __ \\           | | (_)               \n | |__) |   _ _ __ | |_ _ _ __ ___   ___ \n |  _  / | | | '_ \\| __| | '_ ` _ \\ / _ \\\n | | \\ \\ |_| | | | | |_| | | | | | |  __/\n |_|  \\_\\__,_|_| |_|\\__|_|_| |_| |_|\\___|\n";
	WHITE_TEXT
	std::cout << std::endl << "Running Runtime " << RUNTIME_VERSION << std::endl;
	// Interpreter
	rt::liveIntrepretSetup();
	while (true)
	{
		YELLOW_TEXT
		std::cout << ">> ";
		WHITE_TEXT
		std::string input;
		std::getline(std::cin,input);
		rt::liveIntrepret(rt::parse(rt::tokenize(input.c_str()), false));
	}
	return 0;
}