#pragma once

#include <ImGui/imgui.h>

namespace Editor
{
	class EditorContext;

	// The dock area along the bottom of the editor, above the status bar and spanning its full width: the panels whose
	// PanelDesc::DefaultDock is Bottom (Log, Content Browser) and anything docked beside them. It's a dockspace of its
	// own, in a window that takes its height out of the viewport's work area, so the main dockspace fits above it. A
	// splitter along its top edge sets its height.
	class BottomDockArea
	{
	public:
		BottomDockArea() = default;

	public:
		// Each frame, after the status bar and before the main dockspace, whose panels may be docked in here
		void Draw(EditorContext& context);

	private:
		// Pixels, below the splitter; 0 until the first Draw sets it from the work area's height. It's drawn smaller while
		// the work area is too short for it, and back at this height when there's room.
		float m_height = 0.0f;
	};
}
