#pragma once

#include <DebugUtils/DebugServices.h>

#include <string>

namespace Editor
{
	// Which log entries the Log panel shows: a search box and a toggle per channel. The panel only calls Draw and
	// IsShown, so a shared list filter can replace this class without touching the panel's drawing.
	class LogFilter
	{
	public:
		LogFilter();

		// The search box and the channel toggles, on one line
		void Draw();

		// The entry's channel is shown, and its text contains the search (ignoring case)
		bool IsShown(const DebugServices::LogEntry& entry) const;

	private:
		char m_searchBuffer[128] = {};
		// m_searchBuffer as a string, updated when it's edited, so IsShown doesn't build one per entry
		std::string m_search;
		bool m_isChannelShown[GetLogChannelIndex(LogChannel::Count)];
	};
}
