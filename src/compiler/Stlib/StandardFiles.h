#pragma once
#include "../interpreter.h"
#include <iostream>
#include <memory>
#include <variant>
#include <ranges>

// This file defines functions and macros which are useful for the development of the StandardLibraries

namespace rt
{
	constexpr double True = 1; // Represents success
	constexpr double False = 0; // Represents failure

	/// <summary>
	/// Creates a Runtime exception with the given error message.
	/// </summary>
	/// <returns>Exception object</returns>
	inline std::shared_ptr<Object> giveException(const std::string& msg)
	{
		auto exception = std::make_shared<Object>("Exception");
		exception->addMember(msg, "message");
		return exception;
	}
}
