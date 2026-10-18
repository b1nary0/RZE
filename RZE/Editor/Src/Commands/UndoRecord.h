#pragma once

#include <Game/World/GameObject/GameObjectDefinitions.h>
#include <Game/World/GameObject/ObjectGUID.h>

#include <string>

namespace Editor
{
	class EditorContext;

	// How to undo and redo one change a command made. Commands hand these to UndoHistory after they run. A record
	// refers to objects by ObjectGUID rather than by handle, so it still finds an object that was taken out of the
	// scene and put back.
	class UndoRecord
	{
	public:
		virtual ~UndoRecord() = default;

		virtual void Undo(EditorContext& context) = 0;
		virtual void Redo(EditorContext& context) = 0;

		// What the Edit menu calls the step, e.g. "Rename"
		virtual std::string GetName() const = 0;

		// Whether it changes the scene. False for a change elsewhere, like renaming a file: undoing or redoing it doesn't
		// make the scene differ from its file or change the selection, and it stays in the history when another scene
		// loads.
		virtual bool ChangesScene() const { return true; }

		// Within one interaction (e.g. a drag), a later record of the same change to the same target folds into this
		// one: this keeps its own "before" and takes newer's "after". False if newer is a different change.
		virtual bool TryMerge(const UndoRecord& newer) { return false; }

		// The history has dropped this record for good: its step went past the cap, or was undone and then replaced
		// by a new change. For releasing what only undo or redo needed, e.g. destroying objects set aside out of the
		// scene. Not called when the history is cleared, since that's before the scene unloads, which destroys them,
		// nor when the editor closes.
		virtual void OnDropped() {}
	};

	namespace UndoTargets
	{
		// The object with guid, or null if it isn't in the scene
		GameObjectPtr FindObject(ObjectGUID guid);
		// Whether gameObject is alive and in the scene. A command's or panel's handle to an object stays valid while
		// undo has it set aside out of the scene, so check this rather than just for null before changing it.
		bool IsInScene(const GameObjectPtr& gameObject);
		// That object's component of type componentID, or null
		GameObjectComponentPtr<GameObjectComponentBase> FindComponent(ObjectGUID guid, GameObjectComponentID componentID);
	}
}
