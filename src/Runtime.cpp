// Runtime
#include "Runtime.hpp"
#include "compiler/tokenizer.h"
#include "compiler/shared_libs.h"
#include "compiler/parser.h"
#include "compiler/interpreter.h"
#include "compiler/exceptions.h"
// C++
#include <bits/getopt_core.h>
#include <cstdlib>
#include <iostream> 
#include <fstream>
#include <optional>
// C
#include <stdlib.h>
#include <getopt.h>
// Gnu
#include <readline/readline.h>
#include <readline/history.h>

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

static void usage()
{
	std::cout <<
		"usage: Runtime [options] file...\n" // TODO: Don't harcode file name
		"Options:\n"
		"-v\t\tdisplays version information\n"
		"Arguments:\n"
		"file\t\tpath to a Runtime script to be run" << std::endl;
}

int main(int argc, char* argv[])
{
	std::optional<std::string> filePath = std::nullopt;
	int opt;

	// Parse arguments
	while ((opt = getopt(argc, argv, "-:vh")) != -1)
	{
		switch (opt)
		{
		case 'v':
			std::cout << "Runtime " << RUNTIME_VERSION << std::endl;
			exit(EXIT_SUCCESS);
		case 'h':
			usage();
			exit(EXIT_SUCCESS);
		case '?':
			std::cout << "Unknown option: " << static_cast<char>(optopt) << std::endl;
			usage();
			exit(EXIT_FAILURE);
		case ':':
			std::cout << "Mssing argument for " << static_cast<char>(optopt) << std::endl;
			usage();
			exit(EXIT_FAILURE);
		case 1:
			filePath = optarg;
		}
	}
	
	if (filePath) // File input
	{
#ifdef _WIN32
		HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
#endif // _WIN32

		std::ifstream file;
		// Open file
		file.open(filePath.value(), std::fstream::in);
		if (file.fail())
		{
			std::cout << "Unable to open file " << argv[1] << std::endl;
			return EXIT_FAILURE;
		}
		file.seekg(0, std::ios::end);
		size_t size = file.tellg();
		std::string fileText(size, ' ');
		file.seekg(0);
		file.read(&fileText[0], size);
		file.close();

		try
		{
			rt::interpret(rt::parse((rt::tokenize(fileText.c_str(), argv[1])), true), argc, argv);
		}
		catch (ParserException e)
		{
			YELLOW_TEXT
				std::cerr << "ParserException: ";
			WHITE_TEXT
				std::cerr << e.what() << std::endl;
				std::cerr << e.where() << std::endl;
			rt::cleanLibraries();
			return EXIT_FAILURE;
		}
		catch (TokenizerException e)
		{
			YELLOW_TEXT
				std::cerr << "TokenizerException: ";
			WHITE_TEXT
				std::cerr << e.what() << std::endl;
				std::cerr << e.where() << std::endl;
			rt::cleanLibraries();
			return EXIT_FAILURE;
		}
		catch (InterpreterException e)
		{
			YELLOW_TEXT
				std::cerr << "InterpreterException: ";
			WHITE_TEXT
				std::cerr << e.what() << std::endl;
				std::cerr << e.where() << std::endl;
			rt::cleanLibraries();
			return EXIT_FAILURE;
		}
		rt::cleanLibraries();
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
#ifdef _WIN32
			TODO
#else
			char* in = readline("\033[33m>>\033[37m ");
			std::string input(in);
			if (not input.empty())
				add_history(in);
			free(in);
#endif
			if (not input.empty())
			{
				try
				{
					rt::liveIntrepret(rt::parse(rt::tokenize(input.c_str(), "live-input"), false));
				}
				catch (ParserException e)
				{
					YELLOW_TEXT
						std::cerr << "ParserException: ";
					WHITE_TEXT
						std::cerr << e.what() << std::endl;
						std::cerr << e.where() << std::endl;
				}
				catch (TokenizerException e)
				{
					YELLOW_TEXT
						std::cerr << "TokenizerException: ";
					WHITE_TEXT
						std::cerr << e.what() << std::endl;
						std::cerr << e.where() << std::endl;
				}
				catch (InterpreterException e)
				{
					YELLOW_TEXT
						std::cerr << "InterpreterException: ";
					WHITE_TEXT
						std::cerr << e.what() << std::endl;
						std::cerr << e.where() << std::endl;
				}
			};
		}
		rl_clear_history();
		rt::cleanLibraries();
		return EXIT_SUCCESS;
	}
}
