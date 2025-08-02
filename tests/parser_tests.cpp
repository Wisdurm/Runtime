#pragma once

#include "../src/compiler/tokenizer.h"
#include "../src/compiler/parser.h"
// C++
#include <variant>
#include <cassert>
#include <iostream>

//TODO: THIS ENTIRE FILE

namespace rt
{
	/// <summary>
	/// Placeholder values for code locations
	/// </summary>
	static const rt::SourceLocation L;

	void visualizeTree(ast::Expression* ast);
	std::string dot(ast::Expression* ast);

	/// <summary>
	/// Test basic function of the tokenizer
	/// </summary>
	void test_parser_basics()
	{
		std::cout << "Testing parser basics..." << std::endl;

		// Verify operator== works
		ast::Expression* r1 = new ast::Call(L, "Print", { new ast::Literal(L, 2), new ast::Identifier(L, "asd")});
		ast::Expression* r2 = new ast::Call(L, "Print", { new ast::Literal(L, 2), new ast::Identifier(L, "asd")});
		assert(*r1 == *r2);
		delete r1;
		delete r2;
		// Test basic top level statements
		const char* test1 = "i\nAssign(i-0,1)";
		ast::Expression* r3 = new ast::Call(L, "Object", { new ast::Identifier(L,"Main"), new ast::Identifier(L, "i"), new ast::Call(L, "Assign", { new ast::BinaryOperator(L,new ast::Identifier(L, "i"), new ast::Literal(L, 0)), new ast::Literal(L, 1) })});
		assert(*parse(tokenize(test1)) == *r3);
		delete r3;

		std::cout << "Passed parser basics!" << std::endl;
	}
	
	/// <summary>
	/// Tests the parsers ability to parse syntax related to explicit value obtaining
	/// </summary>
	void test_parser_values()
	{
		std::cout << "Testing parser evaluation..." << std::endl;

		const char* test1 = "Human-\"EyeColor\"() # Returns “blue”";
		ast::Expression* r1 = new ast::Call(L, "Object", { new ast::Identifier(L,"Main"), new ast::BinaryOperator(L, new ast::Identifier(L, "Human"), new ast::UnaryOperator(L, new ast::Literal(L, ast::value("EyeColor")) ))});
		assert(*parse(tokenize(test1)) == *r1);
		delete r1;

		const char* test2 = "CarArray-1() # Returns “BMW”";
		ast::Expression* r2 = new ast::Call(L, "Object", { new ast::Identifier(L,"Main"), new ast::BinaryOperator(L, new ast::Identifier(L, "CarArray"), new ast::UnaryOperator(L, new ast::Literal(L, 1))) });
		assert(*parse(tokenize(test2)) == *r2);
		delete r2;

		const char* test3 = "CountCars()()() # Insanity";
		ast::Expression* r3 = new ast::Call(L, "Object", { new ast::Identifier(L,"Main"), new ast::UnaryOperator(L,new ast::UnaryOperator(L, new ast::Call(L, "CountCars", {})))});
		assert(*parse(tokenize(test3)) == *r3);
		delete r3;

		std::cout << "Passed parser evaluation!" << std::endl;
	}

	/// <summary>
	/// Does some more advanced testing of multiline source code
	/// </summary>
	void test_parser_advanced()
	{
		std::cout << "Testing advanced parser tests..." << std::endl;

		const char* test1 = "i \n Object( body, Print(param1), Assign(param1-0, Add(param1(), 1)) )\nWhile(SmallerThan(i, 5), body(i) )";
		ast::Expression* r1 = new ast::Call(L, "Object", { new ast::Identifier(L,"Main"), 
			new ast::Identifier(L, "i"),
			new ast::Call(L, "Object", { new ast::Identifier(L, "body"), new ast::Call(L, "Print", {new ast::Identifier(L, "param1")}), new ast::Call(L, "Assign", {new ast::BinaryOperator(L, new ast::Identifier(L, "param1"),new ast::Literal(L, 0)), new ast::Call(L, "Add", {new ast::Call(L, "param1", {}), new ast::Literal(L, 1)}) })}),
			new ast::Call(L, "While", { new ast::Call(L, "SmallerThan", {new ast::Identifier(L, "i"), new ast::Literal(L, 5)}), new ast::Call(L, "body", {new ast::Identifier(L, "i")})})
			});
		assert(*parse(tokenize(test1)) == *r1);
		delete r1;

		std::cout << "Passed advanced parser tests!" << std::endl;
	}

	/// <summary>
	/// Prints a visual representation of an ast tree to cout
	/// </summary>
	/// <param name="ast"></param>
	static void visualizeTree(ast::Expression* ast)
	{
		// TODO: Use Graphviz for visual graph
	}
	static std::string dot(ast::Expression* ast)
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

			if (std::holds_alternative<std::string>(node->value.valueHeld))
				std::cout << std::get<std::string>(node->value.valueHeld) << std::endl;
			else if (std::holds_alternative<long>(node->value.valueHeld))
				std::cout << std::to_string(std::get<long>(node->value.valueHeld)) << std::endl;
			else
				std::cout << std::to_string(std::get<double>(node->value.valueHeld)) << std::endl;
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