#pragma once

#include <UI/Framework/EditorPanel.h>
#include <UI/Inspector/PropertyInspector.h>

#include <Game/World/GameObject/GameObjectDefinitions.h>

#include <vector>

namespace Editor
{
	// The primary selected GameObject's components, in EditorComponentCache order. Each shows its described properties
	// (ComponentPropertyRegistry), then anything its OnEditorInspect adds.
	class InspectorPanel : public EditorPanel
	{
	public:
		static PanelDesc Describe();

	private:
		void Update(EditorContext& context) override;
		void Draw(EditorContext& context) override;

	private:
		// The primary selection's components in display order. Rebuilt every Update; a member only so its
		// allocation is reused. Handles, because a scene can unload between Update and Draw (e.g. New Scene).
		std::vector<GameObjectComponentPtr<GameObjectComponentBase>> m_sortedComponents;

		PropertyInspector m_propertyInspector;
	};
}
