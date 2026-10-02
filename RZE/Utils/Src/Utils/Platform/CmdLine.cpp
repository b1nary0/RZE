#include <StdAfx.h>
#include <Utils/Platform/CmdLine.h>

#include <Utils/DebugUtils/Debug.h>

#include <charconv>
#include <cstdio>

namespace CmdLine
{
	typedef std::unordered_map<std::string_view, std::string_view> ArgumentMap;
	static ArgumentMap m_arguments;
	static bool s_bInitialized = false;

	// A flag starts with '-' but isn't a negative number, so "-launchMonitor -1" still treats -1 as a value
	bool IsFlag(std::string_view token)
	{
		if (token.size() < 2 || token[0] != '-')
		{
			return false;
		}

		return !(token[1] >= '0' && token[1] <= '9');
	}

	void PartitionString(char** str, int count)
	{
		if (str == nullptr || count < 1)
		{
			return;
		}

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
		int startIndex = 1;
		if (count > 1 && !argStrings[1].empty() && argStrings[1][0] != '-')
		{
			m_arguments["-scene"] = argStrings[1].substr(argStrings[1].find_last_of('\\') + 1, argStrings[1].size());
			startIndex = 2;
		}

		for (int idx = startIndex; idx < count; ++idx)
		{
			const std::string_view token = argStrings[idx];
			if (!IsFlag(token))
			{
				// @note can't use RZE_LOG here, arguments are parsed before the log file is created
				printf_s("Ignoring unexpected command line argument: %.*s\n", static_cast<int>(token.size()), token.data());
				continue;
			}

			// Flags without a value are stored with an empty value so they can still be queried for presence
			const bool bHasValue = (idx + 1 < count) && !IsFlag(argStrings[idx + 1]);
			m_arguments[token] = bHasValue ? argStrings[idx + 1] : std::string_view();
			if (bHasValue)
			{
				++idx;
			}
		}
	}

	namespace Arguments
	{
		void Initialize(char** str, int count)
		{
			if (s_bInitialized)
			{
				// @note can't use RZE_LOG here, arguments are parsed before the log file is created
				printf_s("CmdLine::Arguments::Initialize called more than once. Ignoring.\n");
				AssertMsg(false, "CmdLine::Arguments::Initialize called more than once");
				return;
			}

			s_bInitialized = true;
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

		bool GetInt(const char* argument, int& outVal)
		{
			std::string_view value;
			if (!Get(argument, value) || value.empty())
			{
				return false;
			}

			int parsed = 0;
			const char* const end = value.data() + value.size();
			const std::from_chars_result result = std::from_chars(value.data(), end, parsed);
			if (result.ec != std::errc() || result.ptr != end)
			{
				return false;
			}

			outVal = parsed;
			return true;
		}
	}
}
