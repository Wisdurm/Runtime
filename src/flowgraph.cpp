// Runtime
#include "compiler/tokenizer.h"
#include "compiler/parser.h"

//TODO: Rewrite, Redo, Reformat

namespace rt
{
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
			std::cout << "type: " << (int)node->litValue.type << std::endl;

			if (std::holds_alternative<std::string>(node->litValue.valueHeld))
				std::cout << std::get<std::string>(node->litValue.valueHeld) << std::endl;
			else if (std::holds_alternative<long>(node->litValue.valueHeld))
				std::cout << std::to_string(std::get<long>(node->litValue.valueHeld)) << std::endl;
			else
				std::cout << std::to_string(std::get<double>(node->litValue.valueHeld)) << std::endl;
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