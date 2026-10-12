#pragma once

#include <UI/Framework/EditorPanel.h>
#include <UI/Inspector/PropertyInspector.h>

#include <Game/World/GameObject/GameObjectDefinitions.h>

#include <vector>

namespace Editor
{
	// The selected GameObjects' components, in EditorComponentCache order. Each shows its described properties
	// (ComponentPropertyRegistry), then its custom inspector section if it has one (REGISTER_COMPONENT_INSPECTOR).
	// With several objects selected, only the component types they all have are shown, and edits apply to all of them.
	class InspectorPanel : public EditorPanel
	{
	public:
		static PanelDesc Describe();

	private:
		void Update(EditorContext& context) override;
		void Draw(EditorContext& context) override;

	private:
		// Each entry is one component type the selected objects share: that component on each object, the primary's
		// first. In display order. Rebuilt every Update; handles, because a scene can unload between Update and Draw
		// (e.g. New Scene).
		std::vector<PropertyInspector::ComponentList> m_componentGroups;
		size_t m_selectedCount = 0;

		PropertyInspector m_propertyInspector;
	};
}
