#pragma once

#include <Commands/UndoRecord.h>

#include <UI/Framework/InteractionTracker.h>

#include <memory>

namespace Editor
{
	class EditorContext;

	// A change the editor UI asks for. Panels push commands to EditorContext's CommandQueue instead of changing
	// the scene while they're drawing, and the queue applies them once every panel has drawn. Commands hold
	// handles rather than pointers, so one whose target was destroyed in the meantime (e.g. by an earlier
	// command in the same flush) does nothing.
	//
	// Every change the editor UI makes to the scene goes through a command, since undo only sees what commands
	// record. A command that creates objects indirectly owns them, and records them for undo like any other change.
	class EditorCommand
	{
	public:
		explicit EditorCommand(InteractionID interaction = k_noInteraction)
			: m_interaction(interaction) {}
		virtual ~EditorCommand() = default;

		// Whether it changed what's saved with the scene: false if its target is gone, or it only changed editor state
		// (e.g. an EditorOnly property)
		virtual bool Execute(EditorContext& context) = 0;

		// For logs
		virtual const char* GetName() const = 0;

		// The continuous interaction (e.g. a gizmo drag) this command is one step of, or k_noInteraction for a
		// one-off change. UndoHistory merges a whole interaction into one step.
		InteractionID GetInteraction() const { return m_interaction; }

		// After Execute returns true: how to undo what it did, or null if it can't be undone. A change with no record
		// means the scene can't be undone back to its file (UndoHistory::MarkUnrecordedChange).
		std::unique_ptr<UndoRecord> TakeUndoRecord() { return std::move(m_undoRecord); }

		// Undo and redo, which change the scene by moving through the history rather than adding to it
		virtual bool IsHistoryNavigation() const { return false; }

	protected:
		// For Execute to call once it has made its change
		void SetUndoRecord(std::unique_ptr<UndoRecord> record) { m_undoRecord = std::move(record); }

	private:
		InteractionID m_interaction;
		std::unique_ptr<UndoRecord> m_undoRecord;
	};
}
