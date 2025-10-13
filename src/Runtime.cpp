// Runtime
#include "Runtime.h"
#include "compiler/tokenizer.h"
#include "compiler/parser.h"
#include "compiler/interpreter.h"
// C++
#include <iostream> 
#include <fstream>

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
	if (argc > 1) // File input
	{
		std::ifstream file;
		// Open file
		file.open(argv[1], std::fstream::in);
		if (file.fail())
		{
			std::cout << "Unable to open file " << argv[1];
			return EXIT_FAILURE;
		}
		file.seekg(0, std::ios::end);
		size_t size = file.tellg();
		std::string fileText(size, ' ');
		file.seekg(0);
		file.read(&fileText[0], size);
		file.close();

		rt::interpret(rt::parse((rt::tokenize(fileText.c_str(), argv[1])), true));
		return EXIT_SUCCESS;
	}
	else // Live interpret
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
			std::getline(std::cin, input);
			if (not input.empty())
				rt::liveIntrepret(rt::parse(rt::tokenize(input.c_str(), "live-input"), false));
		}
		return EXIT_SUCCESS;
	}
}