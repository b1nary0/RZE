#pragma once

#include <functional>
#include <string>

namespace Editor
{
	class EditorContext;

	// Asks whether to save unsaved scene changes before something that would lose them (New, Load, Exit, closing
	// the window) or that reads the scene's file (Launch Game): Save, Don't Save or Cancel. Owned by EditorContext;
	// EditorApp displays it each frame.
	class UnsavedChangesPrompt
	{
	public:
		UnsavedChangesPrompt() = default;
		~UnsavedChangesPrompt() = default;

	public:
		// Runs action straight away if the scene has no unsaved changes. Otherwise opens the prompt, and runs it once
		// the user picks Don't Save, or Save and the save goes through (not if they cancel the file browser it opens
		// for a new scene). actionDescription finishes "Save them before ...", e.g. "exiting". A request while the
		// prompt (or its save) is open replaces the one it's asking about.
		void Request(EditorContext& context, std::string actionDescription, std::function<void()> action);

		bool IsOpen() const { return m_pendingAction != nullptr; }

		// Must be called at the root of the ImGui ID stack (outside any Begin/End pair)
		void Display(EditorContext& context);

	private:
		void Finish(bool runAction);

	private:
		std::string m_actionDescription;
		std::function<void()> m_pendingAction;
		bool m_shouldOpen = false;
		// While the file browser is asking where to save, which then decides whether the action runs
		bool m_isSaving = false;
	};
}
