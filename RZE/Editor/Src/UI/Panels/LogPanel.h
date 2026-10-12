#pragma once

#include <UI/Framework/EditorPanel.h>

namespace Editor
{
	// Every DebugServices log entry, following new ones as they arrive
	class LogPanel : public EditorPanel
	{
	public:
		static PanelDesc Describe();

	private:
		void Update(EditorContext& context) override;
		void Draw(EditorContext& context) override;

	private:
		// Entries can arrive from anywhere (RZE_LOG, build output), so the panel follows the count
		size_t m_lastEntryCount = 0;
		bool m_hasNewEntries = false;
	};
}
