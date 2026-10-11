#pragma once

#include <UI/Framework/EditorPanel.h>

#include <vector>

class GameObjectComponentBase;

namespace Editor
{
	// The primary selected GameObject's components, in EditorComponentCache order, each drawn by its OnEditorInspect
	class InspectorPanel : public EditorPanel
	{
	public:
		static PanelDesc Describe();

	private:
		void Draw(EditorContext& context) override;

	private:
		// Rebuilt every frame; a member only so its allocation is reused
		std::vector<GameObjectComponentBase*> m_sortedComponents;
	};
}
