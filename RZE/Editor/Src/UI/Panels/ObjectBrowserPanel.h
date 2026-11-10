#pragma once

#include <UI/Framework/EditorEvents.h>
#include <UI/Framework/EditorPanel.h>

#include <Game/World/GameObject/GameObjectDefinitions.h>

#include <memory>
#include <vector>

namespace Editor
{
	class EditorSelection;

	// The scene's GameObject hierarchy: selection (click, Ctrl+click to toggle, Shift+click for a range), reparenting
	// by drag and drop, renaming in place (F2), and per-object actions (rename, add component, detach, delete) from the
	// context menu. Add GameObject is in its toolbar, and in the menu on its empty space.
	class ObjectBrowserPanel : public EditorPanel
	{
	public:
		static PanelDesc Describe();

		ObjectBrowserPanel();
		~ObjectBrowserPanel() override;

	private:
		void Initialize(EditorContext& context) override;
		void DrawMenuBar(EditorContext& context) override;
		void Draw(EditorContext& context) override;
		void OnSceneReset() override;

		// One object's row, without its children's; returns whether it's open, so its children are rows too
		bool DisplayObject(EditorContext& context, GameObjectPtr gameObject);
		// Selects the rows from the anchor to the Shift+clicked one, which becomes the primary
		void SelectRange(EditorSelection& selection, GameObjectPtr target, bool addToSelection);

		// Puts a text box with the object's name over its row. Enter renames it; Escape or a click elsewhere keeps the
		// name it has. Ends if its row isn't in the tree any more (deleted, or under a collapsed parent).
		void BeginRename(GameObjectPtr gameObject);
		// The rename's text box, over the row from rowMin to rowMax, its name starting at labelX
		void DrawRenameBox(EditorContext& context, const ImVec2& rowMin, const ImVec2& rowMax, float labelX);

	private:
		// The rename in progress, if any
		struct ObjectRename;
		std::unique_ptr<ObjectRename> m_rename;
		// Starts the rename of the object Create GameObject made
		EventSubscription m_gameObjectCreated;

		// Where a Shift+click range starts: the last object clicked without Shift
		GameObjectPtr m_rangeAnchor;
		// The rows the tree shows this frame, top to bottom, those scrolled out of view too; a collapsed parent's
		// children aren't among them. Kept between frames only so its memory is reused, as is m_roots.
		std::vector<GameObjectPtr> m_visibleRows;
		// The scene's root objects this frame, in its order, which are the tree's top level
		std::vector<GameObjectPtr> m_roots;
		// Shift+clicked this frame; the range is selected once every row is drawn, since it can end below the click
		GameObjectPtr m_rangeClickTarget;
		bool m_isRangeAdded = false;
	};
}
