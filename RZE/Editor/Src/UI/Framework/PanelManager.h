#pragma once

#include <UI/Framework/EditorPanel.h>

#include <memory>
#include <vector>

namespace Editor
{
	class EditorContext;

	// Owns every registered EditorPanel and hosts its window: Begin/End, open state, window style, menu entry.
	// Owned by EditorContext.
	class PanelManager
	{
	public:
		PanelManager() = default;
		~PanelManager() = default;

		PanelManager(const PanelManager&) = delete;
		PanelManager& operator=(const PanelManager&) = delete;

	public:
		// Creates and initializes every panel registered with REGISTER_EDITOR_PANEL. Once, after ImGui is set up.
		void CreatePanels(EditorContext& context);

		// Each open panel's window, inside the editor's dockspace
		void DrawAll(EditorContext& context);

		// A checkable item per panel with a PanelDesc::MenuPath, nested in submenus by its path. Call inside the
		// menu the paths are relative to (View); submenus with the same name as existing ones are appended to.
		void DrawMenuItems();

		void ResetSceneState();

		// Null if no panel of that type is registered. Prefer sharing state through EditorContext;
		// this is for the rare case that really needs a specific panel.
		template <typename TPanel>
		TPanel* Find() const;

	private:
		std::vector<std::unique_ptr<EditorPanel>> m_panels;
	};

	template <typename TPanel>
	TPanel* PanelManager::Find() const
	{
		for (const std::unique_ptr<EditorPanel>& panel : m_panels)
		{
			if (TPanel* const found = dynamic_cast<TPanel*>(panel.get()))
			{
				return found;
			}
		}

		return nullptr;
	}
}
