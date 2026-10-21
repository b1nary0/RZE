#pragma once

#include <ImGui/imgui.h>

#include <string>

namespace Editor
{
	class EditorContext;

	// Where a panel docks in the default layout (PanelManager::BuildDefaultDockLayout and BuildBottomDockLayout). An area
	// with two halves is only split when panels use both; panels in just one of them fill it.
	enum class EPanelDock
	{
		Floating,
		// A column down the left side
		Left,
		// A column down the right side, in a top and bottom half
		RightTop,
		RightBottom,
		// The bottom dock area (BottomDockArea), spanning the full width under the others, in a left and right half
		BottomLeft,
		BottomRight,
		// What's left in the middle; the scene view
		Center,

		Count
	};

	// How a panel's window is set up. Each panel class provides one from a static Describe(), which
	// REGISTER_EDITOR_PANEL reads.
	struct PanelDesc
	{
		// The window's title and ImGui ID. imgui.ini saves the docked layout under it, so renaming a panel loses its layout.
		std::string Name;
		// Where its menu item appears, starting at a main menu, e.g. "View/Windows/Log"; empty leaves it out.
		// Items are listed by name.
		std::string MenuPath;
		// Only used when there's no saved layout to restore (imgui.ini)
		EPanelDock DefaultDock = EPanelDock::Floating;
		// For a Floating panel the first time it opens, centred in the editor; zero leaves it to ImGui. Pixels.
		ImVec2 DefaultSize = ImVec2(0.0f, 0.0f);
		bool DefaultOpen = true;
		// Shows the window's close button; panels that can't close are always open
		bool Closable = true;
		// Other panels can be docked into its node, as a tab or split off to one side. Off for the scene view.
		bool AcceptsDocking = true;
		ImGuiWindowFlags WindowFlags = ImGuiWindowFlags_None;
		// Draw() does its own Begin/End (e.g. ImGui's demo window), so PanelManager doesn't wrap it in a window.
		// WindowFlags, PushWindowStyle and DrawMenuBar don't apply, and the panel isn't tracked as hovered or focused.
		bool OwnsWindow = false;
	};

	// Style vars a panel pushes for its window. They're pushed before Begin and popped after End, so they
	// cover the title bar and docking tab as well as the contents.
	class PanelWindowStyle
	{
	public:
		void Push(ImGuiStyleVar var, float value);
		void Push(ImGuiStyleVar var, const ImVec2& value);
		void PopAll();

	private:
		int m_pushCount = 0;
	};

	// Base for an editor window. A panel describes its window with a static Describe() and draws its contents in
	// Draw(); PanelManager owns it and does the window's Begin/End, open state and style. Register a panel by
	// putting REGISTER_EDITOR_PANEL(PanelType) in its .cpp.
	class EditorPanel
	{
		friend class PanelManager;

	public:
		EditorPanel() = default;
		virtual ~EditorPanel() = default;

		EditorPanel(const EditorPanel&) = delete;
		EditorPanel& operator=(const EditorPanel&) = delete;

	public:
		const PanelDesc& GetDesc() const { return m_desc; }

		bool IsOpen() const { return m_isOpen; }
		void SetOpen(bool isOpen) { m_isOpen = isOpen || !m_desc.Closable; }

		// As of the last time the panel was drawn; false while it's closed or hidden behind another tab
		bool IsHovered() const { return m_isHovered; }
		bool IsFocused() const { return m_isFocused; }

	protected:
		// Once, after every panel has been created
		virtual void Initialize(EditorContext& context) {}

		// Each frame while the panel is open, before ImGui's frame starts (EditorApp::PreUIUpdate): gather and
		// prepare what Draw shows. No ImGui calls: there's no ImGui frame yet, so they fail immediately.
		virtual void Update(EditorContext& context) {}

		// The window's contents, between its Begin and End. Only called while the window is open and visible.
		virtual void Draw(EditorContext& context) = 0;

		// Each frame while the panel is open, after the scene's update and before the frame renders
		// (EditorApp::PreRender): what goes into the scene every frame, such as debug lines, which are cleared after each
		// render. No ImGui calls; it's outside ImGui's frame.
		virtual void PreRender(EditorContext& context) {}

		// The window's menu bar, before Draw. Only called when PanelDesc::WindowFlags includes ImGuiWindowFlags_MenuBar.
		virtual void DrawMenuBar(EditorContext& context) {}

		// Before the scene unloads (SceneUnloadingEvent). Drop anything that refers to the current scene.
		virtual void OnSceneReset() {}

		virtual void PushWindowStyle(PanelWindowStyle& style) const {}

		// For PanelDesc::OwnsWindow panels to pass to their own Begin, so its close button closes the panel
		bool* GetOpenFlag() { return &m_isOpen; }

	private:
		PanelDesc m_desc;
		bool m_isOpen = true;
		bool m_isHovered = false;
		bool m_isFocused = false;
	};
}
