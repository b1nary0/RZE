#pragma once

#include <Commands/UndoRecord.h>

#include <UI/Framework/InteractionTracker.h>

#include <Game/World/GameObject/ObjectGUID.h>

#include <deque>
#include <memory>
#include <string>
#include <vector>

namespace Editor
{
	class EditorContext;

	// The steps that can be undone and redone. Owned by EditorContext; CommandQueue::Flush records what each command
	// did, and UndoCommand / RedoCommand step through it. The scene's steps are cleared when it unloads; steps that
	// don't change the scene (UndoRecord::ChangesScene), like renaming a file, stay.
	//
	// A step is everything one interaction did: a gizmo drag over three objects is one step, with one record per
	// object, each holding the transform from before the drag and from the end of it. A step also keeps the
	// selection from before and after it, which undo and redo put back.
	//
	// The save point is where in the history the scene matches its file, which SceneDocument::IsModified is worked
	// out from. Steps that don't change the scene don't move the history off it.
	class UndoHistory
	{
	public:
		static constexpr size_t k_maxSteps = 200;

		UndoHistory() = default;
		~UndoHistory() = default;

		UndoHistory(const UndoHistory&) = delete;
		UndoHistory& operator=(const UndoHistory&) = delete;

	public:
		// Adds what a command did, with the selection (as GUIDs) from before and after it. A record from the same
		// interaction as the last step (other than k_noInteraction) joins that step, unless it was closed, and becomes
		// its "after" selection; otherwise it starts a new one, dropping anything that could be redone, and the oldest
		// step once there are more than k_maxSteps (see UndoRecord::OnDropped).
		void Record(InteractionID interaction, std::unique_ptr<UndoRecord> record,
			std::vector<ObjectGUID> selectionBefore = {}, std::vector<ObjectGUID> selectionAfter = {});

		// The next record starts a new step even if it's from the same interaction
		void CloseStep() { m_isLastStepOpen = false; }

		bool CanUndo() const { return m_position > 0; }
		bool CanRedo() const { return m_position < m_steps.size(); }
		// For the Edit menu, e.g. "Rename"; empty if there's nothing to undo / redo
		std::string GetUndoName() const;
		std::string GetRedoName() const;

		// False if there was nothing to undo / redo. Each also selects what was selected before / after the step,
		// leaving out objects that aren't in the scene, unless the step doesn't change the scene.
		bool Undo(EditorContext& context);
		bool Redo(EditorContext& context);

		// The scene was just saved: it matches its file here. Closes the step, so a change after the save is a new one.
		void MarkSaved();
		// The scene changed in a way the history didn't record, so no point in it matches the file any more
		void MarkUnrecordedChange() { m_savePoint = k_noSavePoint; }
		// Whether the scene matches its file: the history is where it was at the last save or load, or only steps that
		// don't change the scene lie between
		bool IsAtSavePoint() const;

		// Drops the steps that change the scene, without calling UndoRecord::OnDropped, for when the scene is about to
		// unload. The rest stay in order, as undoable or redoable as they were. Where that leaves the history is the save
		// point, since a load follows.
		void Clear();

	private:
		struct Step
		{
			std::string Name;
			InteractionID Interaction = k_noInteraction;
			std::vector<std::unique_ptr<UndoRecord>> Records;
			std::vector<ObjectGUID> SelectionBefore;
			std::vector<ObjectGUID> SelectionAfter;
			// Whether any of its records does (UndoRecord::ChangesScene)
			bool ChangesScene = true;
		};

		static constexpr size_t k_noSavePoint = static_cast<size_t>(-1);

		// Whether any step from first up to (not including) last changes the scene
		bool ChangesSceneBetween(size_t first, size_t last) const;

		static void Drop(Step& step);
		static void RestoreSelection(EditorContext& context, const std::vector<ObjectGUID>& selection);

	private:
		std::deque<Step> m_steps;
		// Steps before this can be undone; the ones from it on can be redone
		size_t m_position = 0;
		bool m_isLastStepOpen = false;
		// The position at the last save or load, or k_noSavePoint once that state can't be reached by undo or redo
		size_t m_savePoint = 0;
	};
}
