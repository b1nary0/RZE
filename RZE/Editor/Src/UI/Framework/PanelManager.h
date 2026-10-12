#pragma once

#include <UI/Framework/EditorEvents.h>
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

		// Each open panel's Update. Before ImGui's frame starts.
		void UpdateAll(EditorContext& context);

		// Each open panel's window, inside the editor's dockspace
		void DrawAll(EditorContext& context);

		// A checkable item per panel whose PanelDesc::MenuPath starts with mainMenu (e.g. "View"), nested in
		// submenus by the rest of its path. Call inside that main menu's BeginMenu/EndMenu; submenus with the same
		// name as existing ones are appended to.
		void DrawMenuItems(const char* mainMenu);

		// Splits the dockspace into the PanelDesc::DefaultDock slots that panels use and docks each panel into
		// its slot. For when there's no saved layout to restore.
		void BuildDefaultDockLayout(ImGuiID dockspaceId, const ImVec2& size);

		// Null if no panel of that type is registered. Prefer sharing state through EditorContext;
		// this is for the rare case that really needs a specific panel.
		template <typename TPanel>
		TPanel* Find() const;

	private:
		std::vector<std::unique_ptr<EditorPanel>> m_panels;
		// Calls each panel's OnSceneReset
		EventSubscription m_sceneUnloading;
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
