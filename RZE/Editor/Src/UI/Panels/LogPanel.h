#pragma once

#include <UI/Framework/EditorPanel.h>
#include <UI/Panels/LogFilter.h>

namespace Editor
{
	// The DebugServices log entries the filter shows, following new ones as they arrive while scrolled to the bottom
	class LogPanel : public EditorPanel
	{
	public:
		static PanelDesc Describe();

	private:
		void PushWindowStyle(PanelWindowStyle& style) const override;
		void DrawMenuBar(EditorContext& context) override;
		void Draw(EditorContext& context) override;

	private:
		LogFilter m_filter;
	};
}
