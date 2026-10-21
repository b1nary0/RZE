#pragma once

#include <ImGui/imgui.h>

struct ImGuiContext;
struct ImRect;
struct ImGuiSettingsHandler;
struct ImGuiWindow;

namespace Editor
{
	class EditorContext;

	// The dock area along the bottom of the editor, above the status bar and spanning its full width: the panels whose
	// PanelDesc::DefaultDock is BottomLeft or BottomRight (Content Browser, Log) and anything docked beside them. It's a dockspace of its
	// own, so it can be drawn in different places without its panels changing dock node. A splitter along its top edge
	// sets its height. The pin at the end of its tab bar switches between:
	//
	// Pinned: always open, in a window that takes its height out of the viewport's work area, so the main dockspace
	// fits above it.
	//
	// Auto-hide: just a strip of tabs, one per panel docked in it. Clicking one opens the area over the bottom of the
	// main dockspace, so the scene view doesn't change size, showing that panel. It stays open while focus is in it
	// (its panels and their popups), and goes back to the strip when focus goes elsewhere. Focusing one of its panels
	// some other way, e.g. Ctrl+Tab, opens it too. While it's closed its panels stay docked, but aren't drawn.
	//
	// Which it is and its height are saved in imgui.ini. It reads ImGui's dock nodes, windows and focus, which an ImGui
	// upgrade may change.
	class BottomDockArea
	{
	public:
		BottomDockArea() = default;

		BottomDockArea(const BottomDockArea&) = delete;
		BottomDockArea& operator=(const BottomDockArea&) = delete;

	public:
		// Once, before ImGui's first frame, which loads imgui.ini
		void Initialize();

		// Each frame, after the status bar and before the main dockspace, whose panels may be docked in here
		void Draw(EditorContext& context);

	private:
		// The area itself, from top to bottom: the splitter, then the dockspace, height tall below it
		void DrawArea(ImGuiID dockspaceId, float height, float heightAbove);
		// Auto-hide's tabs, and the pin while the area's closed. isAreaDrawn: the area's drawn this frame even though it's
		// closed.
		void DrawStrip(ImGuiID dockspaceId, bool isAreaDrawn);
		// At the right end of bar, a tab bar in the current window; switches between pinned and auto-hide
		void DrawPinButton(const ImRect& bar);
		void SetPinned(bool isPinned);

		// Whether focus is in one of its panels, or a popup opened from one
		bool IsFocusInArea(ImGuiID dockspaceId) const;
		// Whether each of its dock nodes has been drawn this session, which ImGui needs before it draws their panels docked
		bool IsHosted(ImGuiID dockspaceId) const;
		// Its window, which isn't there before it's first drawn
		ImGuiWindow* FindWindow() const;

		static void* ReadSettingsOpen(ImGuiContext* context, ImGuiSettingsHandler* handler, const char* name);
		static void ReadSettingsLine(ImGuiContext* context, ImGuiSettingsHandler* handler, void* entry, const char* line);
		static void WriteSettings(ImGuiContext* context, ImGuiSettingsHandler* handler, ImGuiTextBuffer* buffer);

	private:
		// Starts auto-hidden, until imgui.ini says otherwise
		bool m_isPinned = false;
		// Pixels, below the splitter; 0 until the first Draw sets it from the work area's height. It's drawn smaller while
		// the work area is too short for it, and back at this height when there's room.
		float m_height = 0.0f;
		// Auto-hide: open over the main dockspace, as of the last Draw
		bool m_isOpen = false;
	};
}
