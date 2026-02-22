#pragma once

#include "ast.h"
#include <tsl/ordered_map.h>
// C++
#include <unordered_map> // Do testing later on to figure out if a normal map would be better
#include <string>
#include <memory>
#include <variant>
#include <functional>
#include <vector>
#include <algorithm>
#include <any>

// Forward declarations
namespace rt
{
	class Object;
}

/// <summary>
/// Object member type
/// </summary>
using objectOrValue = std::variant<std::shared_ptr<rt::Object>, std::variant<double, std::string>>;

namespace rt {
	/// <summary>
	/// Main class for representing Runtime objects
	/// </summary>
	class Object
	{
	public:
		/// <summary>
		/// Default constructor
		/// </summary>
		Object()
		{
			name = "";
			expr = nullptr;
		}
		/// <summary>
		/// Creates object with expression
		/// </summary>
		/// <param name="expr"></param>
		Object(std::shared_ptr<ast::Expression> expr)
		{
			name = "";
			this->expr = expr;
		}
		/// <summary>
		/// Creates empty object with specified name and expression
		/// </summary>
		Object(std::string name, std::shared_ptr<ast::Expression> expr)
		{
			this->name = name;
			this->expr = expr;
		}
		/// <summary>
		/// Creates empty object with specified name
		/// </summary>
		Object(std::string name)
		{
			this->name = name;
			expr = nullptr;
		}

		/// <summary>
		/// Creates an empty unnamed object with a single value as a member
		/// </summary>
		/// <param name="value"></param>
		Object(std::variant<double, std::string>& value)
		{
			name = "";
			expr = nullptr;
			addMember(value);
		}

		/// <summary>
		/// Return name member
		/// </summary>
		/// <returns></returns>
		std::string* getName() { return &name; };
		/// <summary>
		/// Return expr member
		/// </summary>
		/// <returns></returns>
		std::shared_ptr<ast::Expression> getExpression() const { return expr; };
		/// <summary>
		/// Returns member by index
		/// </summary>
		/// <param name="key">Index</param>
		/// <returns></returns>
		objectOrValue* getMember(int key) 
		{
			// If exists, return
			tsl::ordered_map<int, objectOrValue>::iterator it = members.find(key);
			if (it != members.end()) // Exists
				return &it.value();
			// if not exist, create
			addMember(key);
			return getMember(key);
		};
		/// <summary>
		/// Returns member by name
		/// </summary>
		/// <param name="key">Name</param>
		/// <returns></returns>
		objectOrValue* getMember(std::string name)
		{ 
			// If exists, return
			std::unordered_map<std::string, int>::iterator it = memberStringMap.find(name);
			if (it != memberStringMap.end()) // Exists
				return &members[it->second];
			// if not exist, create
			addMember(name);
			return getMember(name);
		}; // TODO: idfk man

		/// <summary>
		/// Returns member by key
		/// </summary>
		/// <returns></returns>
		objectOrValue* getMember(std::variant<double, std::string> key)
		{
			if (std::holds_alternative<double>(key)) // Number
			{
				int memberKey = static_cast<int>(std::get<double>(key));
				return getMember(memberKey);
			}
			else
			{
				std::string memberKey = std::get<std::string>(key); // String
				return getMember(memberKey);
			}
		}
		/// <summary>
		/// Returns a vector containing all of the members
		/// </summary>
		/// <returns></returns>
		std::vector<objectOrValue> getMembers()
		{
			std::vector<objectOrValue> r;
			for (tsl::ordered_map<int, objectOrValue>::iterator it = members.begin(); it != members.end(); ++it)
			{
				r.push_back(it->second);
			};
			return r; 
		};
		/// <summary>
		/// Add member with just int key
		/// </summary>
		/// <param name="key"></param>
		void addMember(int key)
		{
			members.insert({key, std::make_shared<Object>()});
		}
		/// <summary>
		/// Add member with just string key
		/// </summary>
		/// <param name="key"></param>
		void addMember(std::string key)
		{
			if (not members.contains(counter))
			{
				members.insert({counter, std::make_shared<Object>() });
				memberStringMap.insert({ key, counter });
				counter++;
			}
			else
			{
				// :(
				counter++;
				this->addMember(key);
			}
		}
		/// <summary>
		/// Adds member with int key
		/// </summary>
		/// <param name="member"></param>
		/// <param name="key"></param>
		void addMember(objectOrValue member, int key) { 
			if (key == counter) 
				counter++;
			members.insert({ key, member });
		};

		/// <summary>
		/// Add member with no key
		/// </summary>
		/// <param name="member"></param>
		void addMember(objectOrValue member)
		{
			if (not members.contains(counter))
			{
				members.insert({counter, member});
				counter++;
			}
			else
			{
				// :(
				counter++;
				this->addMember(member);
			}
		}
		/// <summary>
		/// Adds member with string key
		/// </summary>
		/// <param name="member"></param>
		/// <param name="key"></param>
		void addMember(objectOrValue member, std::string key) {
			if (not members.contains(counter))
			{
				members.insert({counter, member});
				memberStringMap.insert({key, counter});
				counter++;
			}
			else
			{
				// :(
				counter++;
				this->addMember(member, key);
			}
		};
		/// <summary>
		/// Sets the value of "key" to "value"
		/// </summary>
		/// <param name="key"></param>
		/// <param name="value"></param>
		void setMember(std::variant<double, std::string> key, objectOrValue value)
		{
			// Delete old member and add new one because no assignment operator idk don't feel like figuring that out :/
			// TODO: Probably not particularly hard to fix, at least anymore
			if (std::holds_alternative<double>(key)) // Number
			{
				// DOES NOT REMOVE STRING KEY MAP TODO TODO TODO
				int memberKey = static_cast<int>(std::get<double>(key));
				if (members.contains(memberKey))
					members.erase(memberKey);
				addMember(value, memberKey);
			}
			else
			{
				std::string memberKey = std::get<std::string>(key); // String
				if (memberStringMap.contains(memberKey))
				{
					members.erase(memberStringMap.at(memberKey));
					memberStringMap.erase(memberKey);
				}
				addMember(value, memberKey);
			}
		}
		/// <summary>
		/// Sets the value of the last member (the one which would be evaluated) to a value
		/// </summary>
		/// <param name="key"></param>
		/// <param name="value"></param>
		void setLast(objectOrValue value)
		{
			// Delete old member and add new one because no assignment operator idk don't feel like figuring that out :/
			// TODO
			if (members.size() > 0)
			{
				// TODO DOES NOT REMOVE STRING KEY, ADD METHOD I GUESS?
				int key = members.back().first;
				members.erase(members.end() - 1); // Remove last one
				addMember(value, key);
			} else throw; // Bored
		}
		/// <summary>
		/// Deletes the expression of this object
		/// </summary>
		void deleteExpression()
		{
			expr.reset();
		}
		/// <summary>
		/// Creates an object from an std::variant<double, std::string>
		/// </summary>
		/// <param name=""></param>
		/// <returns></returns>
		static std::shared_ptr<Object> objectFromValue(std::variant<double, std::string> value)
		{
			// TODO: Might not be needed, depends...
			// Remember the constructor associated with this!
			return std::make_shared<Object>(value);
		}
	private:
		/// <summary>
		/// Name of the object
		/// </summary>
		std::string name;
		/// <summary>
		/// (Optional) expression value. Can be evaluated
		/// </summary>
		std::shared_ptr<ast::Expression> expr;
		/// <summary>
		/// Members of the object. Can either be objects, or values
		/// </summary>
		tsl::ordered_map<int, objectOrValue> members;
		/// <summary>
		/// Maps strings to their placement on the members list. Definitely not the best way to implement this, but I can always change it later
		/// </summary>
		std::unordered_map<std::string, int> memberStringMap;
		/// <summary>
		/// Counts up the indexing of new members
		/// </summary>
		int counter = 0;
	};
}
