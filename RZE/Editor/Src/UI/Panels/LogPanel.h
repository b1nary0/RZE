#pragma once

#include <Utils/PrimitiveDefs.h>

#include <deque>
#include <string>

namespace Editor
{
	class LogPanel
	{
	public:
		LogPanel() = default;
		~LogPanel() = default;

	public:
		void Display();

	public:
		void AddEntry(const std::string& msg);
	};
}