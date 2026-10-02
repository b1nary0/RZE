#pragma once

#include <unordered_map>
#include <string_view>

namespace CmdLine
{
	namespace Arguments
	{
		// May only be called once per process. Later calls assert in debug and are ignored.
		// str must outlive all use of CmdLine (pass main's argv), stored values point into it.
		void Initialize(char** str, int count);
		
		bool Get(const char* argument, std::string_view& outVal);

		// Returns false if the argument is missing or its value isn't a valid integer. outVal is untouched on failure.
		bool GetInt(const char* argument, int& outVal);
	};
}