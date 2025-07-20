#pragma once
// This file contains all the functions for the StandardLibrary in Runtime
#include "../interpreter.h"
// C++
#include <iostream>

namespace rt
{
	/// <summary>
	/// Prints a value to the standard output
	/// </summary>
	/// <param name="str">Value to print</param>
	/// <param name="capture">Whether or not to capture the output to a vector</param>
	void Print(std::string str, bool capture)
	{
		if (capture)
			captureString(str);
		std::cout << str;
	}
}