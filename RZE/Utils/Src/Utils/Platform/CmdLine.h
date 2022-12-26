#pragma once

#include <unordered_map>
#include <string_view>

namespace CmdLine
{
	namespace Arguments
	{
		void Initialize(char** str, int count);
		
		bool Get(const char* argument, std::string_view& outVal);
	};
}