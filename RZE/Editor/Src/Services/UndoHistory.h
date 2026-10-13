#pragma once

#include <Commands/UndoRecord.h>

#include <UI/Framework/InteractionTracker.h>

#include <deque>
#include <memory>
#include <string>
#include <vector>

namespace Editor
{
	class EditorContext;

	// The steps that can be undone and redone. Owned by EditorContext; CommandQueue::Flush records what each command
	// did, and UndoCommand / RedoCommand step through it. Cleared when the scene unloads.
	//
	// A step is everything one interaction did: a gizmo drag over three objects is one step, with one record per
	// object, each holding the transform from before the drag and from the end of it.
	class UndoHistory
	{
	public:
		static constexpr size_t k_maxSteps = 200;

		UndoHistory() = default;
		~UndoHistory() = default;

		UndoHistory(const UndoHistory&) = delete;
		UndoHistory& operator=(const UndoHistory&) = delete;

	public:
		// Adds what a command did. A record from the same interaction as the last step (other than k_noInteraction)
		// joins that step, unless it was closed; otherwise it starts a new one, dropping anything that could be redone,
		// and the oldest step once there are more than k_maxSteps.
		void Record(InteractionID interaction, std::unique_ptr<UndoRecord> record);

		// The next record starts a new step even if it's from the same interaction
		void CloseStep() { m_isLastStepOpen = false; }

		bool CanUndo() const { return m_position > 0; }
		bool CanRedo() const { return m_position < m_steps.size(); }
		// For the Edit menu, e.g. "Rename"; empty if there's nothing to undo / redo
		std::string GetUndoName() const;
		std::string GetRedoName() const;

		// False if there was nothing to undo / redo
		bool Undo(EditorContext& context);
		bool Redo(EditorContext& context);

		void Clear();

	private:
		struct Step
		{
			std::string Name;
			InteractionID Interaction = k_noInteraction;
			std::vector<std::unique_ptr<UndoRecord>> Records;
		};

		std::deque<Step> m_steps;
		// Steps before this can be undone; the ones from it on can be redone
		size_t m_position = 0;
		bool m_isLastStepOpen = false;
	};
}
