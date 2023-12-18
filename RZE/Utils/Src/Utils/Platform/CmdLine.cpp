#include <Utils/StdAfx.h>
#include <Utils/Platform/CmdLine.h>

namespace CmdLine
{
	typedef std::unordered_map<std::string_view, std::string_view> ArgumentMap;
	static ArgumentMap m_arguments;

	void PartitionString(char** str, int count)
	{
		std::vector<std::string_view> argStrings;
		argStrings.reserve(count);

		for (int argIndex = 0; argIndex < count; ++argIndex)
		{
			argStrings.emplace_back(str[argIndex]);
		}

		m_arguments.reserve(argStrings.size());
		m_arguments["-exe"] = argStrings[0];

		// #TODO hacky here to check for file being sent in from windows when a file is double clicked...
		// there must be a better, more standard way to handle this..
		// #NOTE this is actually broken... if the first argument passed is not -scene then this breaks..?
		int startIndex = 1;
		if (argStrings[1][0] != '-')
		{
			m_arguments["-scene"] = argStrings[1].substr(argStrings[1].find_last_of('\\') + 1, argStrings[1].size());
			startIndex = 2;
		}

		for (size_t idx = startIndex; idx < argStrings.size();)
		{
			if (argStrings[idx][0] == '-')
			{
				std::string_view key = argStrings[idx];
				std::string_view value = argStrings[idx + 1];

				m_arguments[key] = value;

				idx += 2;
			}
		}
	}

	namespace Arguments
	{
		void Initialize(char** str, int count)
		{
			PartitionString(str, count);
		}

		bool Get(const char* argument, std::string_view& outVal)
		{
			const auto& iter = m_arguments.find(argument);
			if (iter != m_arguments.end())
			{
				outVal = iter->second;
				return true;
			}

			return false;
		}
	}
}
