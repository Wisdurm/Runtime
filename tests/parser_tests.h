#pragma once

#include "../src/compiler/tokenizer.h"
#include "../src/compiler/parser.h"
// C++
#include <cassert>
#include <iostream>

namespace rt
{
	void visualizeTree(ast::Expression* ast);
	std::string dot(ast::Expression* ast);

	/// <summary>
	/// Test basic function of the tokenizer
	/// </summary>
	void test_parser_basics()
	{
		// Verify operator== works
		ast::Expression* r1 = new ast::Call(SourceLocation(), "Print", { new ast::Literal(SourceLocation(), 2), new ast::Identifier(SourceLocation(), "asd")});
		ast::Expression* r2 = new ast::Call(SourceLocation(), "Print", { new ast::Literal(SourceLocation(), 2), new ast::Identifier(SourceLocation(), "asd")});
		assert(*r1 == *r2);
		delete r1;
		delete r2;
		// Basics
		const char* test1 = "Print(\"Hello world!\")";
		ast::Expression* r3 = new ast::Call(SourceLocation(), "Print", { new ast::Literal(SourceLocation(), ast::value("Hello world!")) });
		assert(*parse(tokenize(test1)) == *r3);
		delete r3;
	}

	/// <summary>
	/// Prints a visual representation of an ast tree to cout
	/// </summary>
	/// <param name="ast"></param>
	void visualizeTree(ast::Expression* ast)
	{
		// TODO: Use Graphviz for visual graph
	}
	std::string dot(ast::Expression* ast)
	{
		// Quick way to print all info from object
		if (dynamic_cast<ast::Identifier*>(ast) != nullptr)
		{
			auto node = dynamic_cast<ast::Identifier*>(ast);
			std::cout << "iden: " << node->name << std::endl;
		}
		else if (dynamic_cast<ast::Literal*>(ast) != nullptr)
		{
			auto node = dynamic_cast<ast::Literal*>(ast);
			std::cout << "type: " << (int)node->value.type << std::endl;
			std::cout << "dec: " << node->value.valueDec << std::endl;
			std::cout << "int: " << node->value.valueInt << std::endl;
			std::cout << "str: " << node->value.valueStr << std::endl;
		}
		else if (dynamic_cast<ast::Call*>(ast) != nullptr)
		{
			auto node = dynamic_cast<ast::Call*>(ast);
			std::cout << node->name << std::endl;
			for (auto arg : node->args)
			{
				dot(arg);
			}
		}
		else if (dynamic_cast<ast::BinaryOperator*>(ast) != nullptr)
		{
			auto node = dynamic_cast<ast::BinaryOperator*>(ast);
			dot(const_cast<ast::Expression*>(node->left));
			dot(const_cast<ast::Expression*>(node->right));
		}
		else if (dynamic_cast<ast::UnaryOperator*>(ast) != nullptr)
		{
			auto node = dynamic_cast<ast::UnaryOperator*>(ast);
			dot(const_cast<ast::Expression*>(node->expr));
		}
		return "";
	}
}