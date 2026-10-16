#pragma once

#include <Utils/Math/Vector3D.h>

namespace Editor
{
	class EditorContext;

	// The editor's main menu bar: File (scenes, building the game), Edit (copy and paste), the View and Help items
	// panels register, and frame time and render memory readouts. Also handles the Edit menu's keyboard shortcuts.
	class MainMenuBar
	{
	public:
		MainMenuBar() = default;
		~MainMenuBar() = default;

	public:
		void Draw(EditorContext& context);

	private:
		void DrawFileMenu(EditorContext& context);
		void DrawEditMenu(EditorContext& context);
		// Ctrl+C/V/D, unless a focused widget (e.g. a text field) takes them first
		void HandleEditShortcuts(EditorContext& context);

		void Copy(EditorContext& context);
		void Paste(EditorContext& context);
		void Duplicate(EditorContext& context);
		// How far a paste moves the pasted objects from the originals, in world space: steps times the copied
		// objects' width (GameObjectClipboard::MeasureWidth) plus a gap, along +X
		static Vector3D GetPasteOffset(const EditorContext& context, float width, int steps);
		// The render command arena's use. Frame times are in the status bar.
		void DrawStatus();
	};
}
