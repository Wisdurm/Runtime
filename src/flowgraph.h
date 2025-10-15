#pragma once
#if RUNTIME_DEBUG==1
namespace rt
{
	std::string treeViz(ast::Expression* ast);
}
#endif