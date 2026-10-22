#pragma once

#include <UI/Framework/EditorPanel.h>

#include <Game/World/GameObject/GameObjectDefinitions.h>

#include <vector>

namespace Editor
{
	class EditorSelection;

	// The scene's GameObject hierarchy: selection (click, Ctrl+click to toggle, Shift+click for a range), reparenting
	// by drag and drop, and per-object actions (rename, add component, detach, delete) from the context menu
	class ScenePanel : public EditorPanel
	{
	public:
		static PanelDesc Describe();

	private:
		void Draw(EditorContext& context) override;
		void OnSceneReset() override;

		void DisplayObject(EditorContext& context, GameObjectPtr gameObject);
		// Selects the rows from the anchor to the Shift+clicked one, which becomes the primary
		void SelectRange(EditorSelection& selection, GameObjectPtr target, bool addToSelection);

	private:
		// Whether the name prompt for a new GameObject is showing
		bool m_isCreatingGameObject = false;

		// Where a Shift+click range starts: the last object clicked without Shift
		GameObjectPtr m_rangeAnchor;
		// The rows drawn this frame, top to bottom; a collapsed parent's children aren't among them.
		// Kept between frames only so its memory is reused.
		std::vector<GameObjectPtr> m_visibleRows;
		// Shift+clicked this frame; the range is selected once every row is drawn, since it can end below the click
		GameObjectPtr m_rangeClickTarget;
		bool m_isRangeAdded = false;
	};
}
