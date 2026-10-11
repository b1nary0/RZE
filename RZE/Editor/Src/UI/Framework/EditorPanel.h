#pragma once

#include <ImGui/imgui.h>

#include <string>

namespace Editor
{
	class EditorContext;

	// How a panel's window is set up. Each panel class provides one from a static Describe(), which
	// REGISTER_EDITOR_PANEL reads.
	struct PanelDesc
	{
		// The window's title and ImGui ID. imgui.ini saves the docked layout under it, so renaming a panel loses its layout.
		std::string Name;
		// Where it appears under the View menu, e.g. "Windows/Log"; empty leaves it out. Items are listed by name.
		std::string MenuPath;
		bool DefaultOpen = true;
		// Shows the window's close button; panels that can't close are always open
		bool Closable = true;
		ImGuiWindowFlags WindowFlags = ImGuiWindowFlags_None;
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

		// The window's contents, between its Begin and End. Only called while the window is open and visible.
		virtual void Draw(EditorContext& context) = 0;

		// Before the scene unloads (EditorContext::ResetSceneState). Drop anything that refers to the current scene.
		virtual void OnSceneReset() {}

		virtual void PushWindowStyle(PanelWindowStyle& style) const {}

	private:
		PanelDesc m_desc;
		bool m_isOpen = true;
		bool m_isHovered = false;
		bool m_isFocused = false;
	};
}
