#pragma once

#include <UI/Framework/EditorEvents.h>

#include <Game/World/GameObject/GameObjectDefinitions.h>

#include <vector>

namespace Editor
{
	// Shows the mesh bounds of the selected GameObjects, through RenderComponent's own toggle so its checkbox in the
	// Object Inspector stays in sync. Follows SelectionChangedEvent. Owned by EditorContext.
	class SelectionBoundsDisplay
	{
	public:
		explicit SelectionBoundsDisplay(EditorEvents& events);
		~SelectionBoundsDisplay() = default;

		SelectionBoundsDisplay(const SelectionBoundsDisplay&) = delete;
		SelectionBoundsDisplay& operator=(const SelectionBoundsDisplay&) = delete;

	private:
		void OnSelectionChanged(const std::vector<GameObjectPtr>& selection);

	private:
		EventSubscription m_selectionChanged;
		// The objects whose bounds this turned on, to turn off again once they're deselected
		std::vector<GameObjectPtr> m_shown;
	};
}
