#pragma once

#include <UI/Framework/EditorPanel.h>

namespace Editor
{
	// Dear ImGui's own demo window, for looking up widgets and styles while building editor UI
	class ImGuiDemoPanel : public EditorPanel
	{
	public:
		static PanelDesc Describe();

	private:
		void Draw(EditorContext& context) override;
	};
}
