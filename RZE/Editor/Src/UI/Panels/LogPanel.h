#pragma once

#include <UI/Framework/EditorPanel.h>

namespace Editor
{
	// Every DebugServices log entry, following new ones as they arrive while scrolled to the bottom
	class LogPanel : public EditorPanel
	{
	public:
		static PanelDesc Describe();

	private:
		void Draw(EditorContext& context) override;
	};
}
