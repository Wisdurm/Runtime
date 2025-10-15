// This was supposed to be something more significant, but it's clear now that it's
// not really a fit for what this project has become
#if RUNTIME_DEBUG==1
// Runtime
#include "compiler/tokenizer.h"
#include "compiler/parser.h"
#include "flowgraph.h"
// C++
#include <iostream>
//TODO: Rewrite, Redo, Reformat

namespace rt
{
	std::string treeViz(ast::Expression* ast)
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
			std::cout << "type: " << (int)node->litValue.type << std::endl;

			if (std::holds_alternative<std::string>(node->litValue.valueHeld))
				std::cout << std::get<std::string>(node->litValue.valueHeld) << std::endl;
			else
				std::cout << std::to_string(std::get<double>(node->litValue.valueHeld)) << std::endl;
		}
		else if (dynamic_cast<ast::Call*>(ast) != nullptr)
		{
			auto node = dynamic_cast<ast::Call*>(ast);
			treeViz(node->object);
			for (auto arg : node->args)
			{
				treeViz(arg);
			}
		}
		else if (dynamic_cast<ast::BinaryOperator*>(ast) != nullptr)
		{
			auto node = dynamic_cast<ast::BinaryOperator*>(ast);
			treeViz(const_cast<ast::Expression*>(node->left));
			treeViz(const_cast<ast::Expression*>(node->right));
		}
		return "";
	}
}
#endif