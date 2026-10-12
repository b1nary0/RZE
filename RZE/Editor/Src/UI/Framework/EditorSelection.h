#pragma once

#include <Game/World/GameObject/GameObjectDefinitions.h>

#include <vector>

namespace Editor
{
	class EditorEvents;

	// The editor's selected GameObjects, shared by every panel through EditorContext.
	// In selection order; the last entry is the primary, which the gizmo and inspector act on.
	// Each call that changes it publishes one SelectionChangedEvent (see SelectionBoundsDisplay).
	class EditorSelection
	{
	public:
		explicit EditorSelection(EditorEvents& events);
		~EditorSelection() = default;

		EditorSelection(const EditorSelection&) = delete;
		EditorSelection& operator=(const EditorSelection&) = delete;

	public:
		bool IsEmpty() const { return m_selection.empty(); }
		// Null when nothing is selected
		GameObjectPtr GetPrimary() const;
		bool IsSelected(const GameObjectPtr& gameObject) const;
		const std::vector<GameObjectPtr>& GetAll() const { return m_selection; }

		void Clear();
		// Replaces the selection
		void Select(GameObjectPtr gameObject);
		// Replaces the selection; duplicates are dropped
		void SetSelection(const std::vector<GameObjectPtr>& gameObjects);
		// Ctrl+click: adds the object, or removes it if it's already selected
		void Toggle(GameObjectPtr gameObject);
		void AddToSelection(const std::vector<GameObjectPtr>& gameObjects);

	private:
		void PublishChanged();

	private:
		EditorEvents& m_events;
		std::vector<GameObjectPtr> m_selection;
	};
}
