#include "Runtime.hpp"
#include "interpreter.h"
#include "shared_libs.h"
#include "symbol_table.h"
#include "object.h"
#include "exceptions.h"
#include "Stlib/StandardLibrary.h"
#include "Stlib/StandardMath.h"
#include "Stlib/StandardIO.h"
// C++
#include <ffi.h>
#include <memory>
#include <variant>
#include <vector>
#include <unordered_set>
#include <cstring>
// C
#include <cstdlib>

namespace rt
{
        // Arg state

	objectOrValue* ArgState::getArg()
	{
		// If have args yet to be assigned
		if (static_cast<size_t>(pos) < args.size())
		{
			pos++;
			return &args.at(static_cast<size_t>(pos) - 1);
		}
		else if (parent != nullptr)
		{
			return parent->getArg();
		}
		else
			return nullptr;
	}

	// Symbol table
	Symbol& SymbolTable::lookUp(std::string key, ArgState& args)
	{
		// Check if key exists
		std::unordered_map<std::string, Symbol>::iterator it = locals.find(key);
		if (it != locals.end()) // Exists
			return it->second;

		auto p = parent;
		while (p != nullptr) // Look for key in parent symbol table
		{
			// Check if key exists
			std::unordered_map<std::string, Symbol>::iterator it = p->locals.find(key);
			if (it != p->locals.end()) // Exists
				return it->second;
			else
				p = p->parent;
		}

		// Cannot find, create symbol
#if RUNTIME_DEBUG==1
		std::cout << "Empty object initialized" << std::endl;
#endif // RUNTIME_DEBUG
		auto v = args.getArg();
		if (v != nullptr)
		{
			if (std::holds_alternative<std::shared_ptr<Object>>(*v)) // If argument is reference
				updateSymbol(key, std::get<std::shared_ptr<Object>>(*v));
			else // Argument is value
			{
				std::shared_ptr<Object> obj = std::make_shared<Object>(std::get<std::variant<double, std::string>>(*v));
				updateSymbol(key, obj);
			}
		}
		else
		{
			std::shared_ptr<Object> zero = std::make_shared<Object>(key);
			updateSymbol(key, zero);
		}
		return lookUp(key, args);
	}

	Symbol& SymbolTable::lookUpHard(std::string key)
	{
		// Check if key exists
		std::unordered_map<std::string, Symbol>::iterator it = locals.find(key);
		if (it != locals.end()) // Exists
			return it->second;

		auto p = parent;
		while (p != nullptr) // Look for key in parent symbol table
		{
			// Check if key exists
			std::unordered_map<std::string, Symbol>::iterator it = p->locals.find(key);
			if (it != p->locals.end()) // Exists
				return it->second;
			else
				p = p->parent;
		}
		// Cannot find, throw
		throw InterpreterException("Unable to find symbol", 0, "Unknown");
	}

	bool SymbolTable::contains(std::string& key) const
	{
		// Check if key exists
		if (locals.contains(key))
			return true;

		auto p = parent;
		while (p != nullptr) // Look for key in parent symbol table
		{
			// Check if key exists
			if (p->locals.contains(key)) // Exists
				return true;
			else
				p = p->parent;
		}
		return false;
	}

	void SymbolTable::updateSymbol(const std::string& key, const std::shared_ptr<Object> object)
	{
		// Check if key exists
		std::unordered_map<std::string, Symbol>::iterator it = locals.find(key);
		if (it != locals.end()) // Exists
			it->second = object; // I don't think there's any chance this works but I'm a bit tired atm so I'll figure this out in the coming days/weeks
		else
			locals.insert({ key, object });
	}

	void SymbolTable::updateSymbol(const std::string& key, LibFunc object)
	{
		// Check if key exists
		std::unordered_map<std::string, Symbol>::iterator it = locals.find(key);
		if (it != locals.end()) // Exists
			it->second = object; // I don't think there's any chance this works but I'm a bit tired atm so I'll figure this out in the coming days/weeks
		else
			locals.insert({ key, object });
	}

	void SymbolTable::clear()
	{
		locals.clear();
		if (parent != nullptr)
			parent->clear();
	}
		
	std::vector<std::string> SymbolTable::getKeys()
	{
		std::vector<std::string> keys;
		// Get from current
		for (auto kv : this->locals) {
			keys.push_back(kv.first);
		}
		// Get from parents
		auto p = parent;
		while (p != nullptr)
		{
			for (auto kv : p->locals) {
				keys.push_back(kv.first);
			}
		}
		return keys;
	}

	void clearSymtab(SymbolTable& symtab)
	{
		symtab = SymbolTable({
			{"Return", Return },
			{"Print", Print },
			{"Input", Input },
			{"Object", ObjectF },
			{"Append", Append },
			{"Copy", Copy },
			{"Assign", Assign},
			{"Update", Update },
			{"Exit", Exit},
			{"Include", Include},
			{"Not", Not},
			{"If", If},
			{"While", While},
			{"Format", Format},
			{"Bind", Bind},
			{"System", System},
			{"GetKeys", GetKeys},
			{"Size", Size},
			// Math
			{"Add", Add},
			{"Minus", Minus},
			{"Multiply", Multiply},
			{"Divide", Divide},
			{"Mod", Mod},
			{"Equal", Equal},
			{"LargerThan", LargerThan},
			{"SmallerThan", SmallerThan},
			{"Sine", Sine},
			{"Cosine", Cosine},
			{"Tangent", Tangent},
			{"ArcSine", ArcSine},
			{"ArcCosine", ArcCosine},
			{"ArcTangent", ArcTangent},
			{"ArcTangent2", ArcTangent2},
			{"Power", Power},
			{"SquareRoot", SquareRoot},
			{"Floor", Floor},
			{"Ceiling", Ceiling},
			{"Round", Round},
			{"NaturalLogarithm", NaturalLogarithm},
			{"RandomDecimal", RandomDecimal},
			{"RandomInteger", RandomInteger},
			// I/O
			{"FileCreate", FileCreate},
			{"FileOpen", FileOpen},
			{"FileClose", FileClose},
			{"FileReadLine", FileReadLine},
			{"FileWrite", FileWrite},
			{"FileAppendLine", FileAppendLine},
			{"FileRead", FileRead},
		});
	}
}
