#pragma once

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
		void DrawStatus();

	private:
		// The frame time readout only updates every few frames so it's readable
		int m_framesSinceSample = 0;
		float m_sampledFrameTimeMS = 0.0f;
	};
}
