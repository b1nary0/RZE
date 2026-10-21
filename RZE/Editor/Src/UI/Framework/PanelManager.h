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

		// Each open panel's PreRender. After the scene's update, outside ImGui's frame.
		void PreRenderAll(EditorContext& context);

		// A checkable item per panel whose PanelDesc::MenuPath starts with mainMenu (e.g. "View"), nested in
		// submenus by the rest of its path. Call inside that main menu's BeginMenu/EndMenu; submenus with the same
		// name as existing ones are appended to.
		void DrawMenuItems(const char* mainMenu);

		// Splits the main dockspace into the PanelDesc::DefaultDock slots that panels use, other than the bottom area's,
		// and docks each panel into its slot. For when there's no saved layout to restore.
		void BuildDefaultDockLayout(ImGuiID dockspaceId, const ImVec2& size);
		// Makes the bottom area's dockspace (BottomDockArea), split into the BottomLeft and BottomRight slots that panels
		// use, and docks their panels into it, for when imgui.ini has no saved layout for it. A layout saved before the bottom area had a dockspace of its own has them in the main
		// dockspace; they're taken out of it, along with the nodes they leave empty.
		void BuildBottomDockLayout(ImGuiID dockspaceId, const ImVec2& size);

		// Null if no panel of that type is registered. Prefer sharing state through EditorContext;
		// this is for the rare case that really needs a specific panel.
		template <typename TPanel>
		TPanel* Find() const;

	private:
		// The panels the default layout docks into slot, in their tab order there: open ones first, then by name
		std::vector<const EditorPanel*> GetPanelsInSlot(EPanelDock slot) const;
		// Splits an area's node between its two halves, first towards firstDir, if panels use both; otherwise the one
		// that's used gets the whole node. A half no panel uses is left 0.
		void SplitArea(ImGuiID areaId, EPanelDock first, EPanelDock second, ImGuiDir firstDir, ImGuiID& firstId, ImGuiID& secondId) const;
		// Whether any panel, open or not, docks in slot by default
		bool IsSlotUsed(EPanelDock slot) const;
		// Docks slot's panels into the node as tabs, in their order there, with the first shown
		void DockSlotPanels(EPanelDock slot, ImGuiID nodeId) const;
		// Before the first Begin of a docked panel that imgui.ini has no place for, e.g. one added since the layout was
		// saved: docks it into the node of a saved panel from its PanelDesc::DefaultDock slot, at its place in the
		// slot's tab order, as the tab that's shown. Does nothing for a panel that has a place, or whose slot has no
		// saved panel.
		void PlaceUnsavedPanel(const EditorPanel& panel);
		// Between BeginMenuBar and EndMenuBar: the panel's menu bar contents, placed by its PanelDesc::MenuBarPadding and
		// CentreMenuBar
		void DrawPanelMenuBar(EditorPanel& panel, EditorContext& context);

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
