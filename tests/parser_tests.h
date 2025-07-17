#pragma once

#include "../src/compiler/tokenizer.h"
#include "../src/compiler/parser.h"
// C++
#include <cassert>

#include <iostream>

namespace rt
{
	/// <summary>
	/// Test basic function of the tokenizer
	/// </summary>
	void test_parser_basics()
	{
		ast::Expression* r1 = new ast::Literal(SourceLocation(), ast::value(2));
		ast::Expression* r2 = new ast::Literal(SourceLocation(), ast::value(2));
		assert(*r1 == *r2);
		delete r1;
		delete r2;

		//const char* test1 = "Import(StandardLibrary)\n# I can eat glass and it doesn't hurt me \nObject(\"Main\", Zero-0)\n";
		//ast::Expression r1 = parse(tokenize(test1));
		//assert(parse(tokenize(test1)) == r1);
	}
}