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

		// Within one interaction (e.g. a drag), a later record of the same change to the same target folds into this
		// one: this keeps its own "before" and takes newer's "after". False if newer is a different change.
		virtual bool TryMerge(const UndoRecord& newer) { return false; }
	};

	namespace UndoTargets
	{
		// The object with guid, or null if it isn't in the scene
		GameObjectPtr FindObject(ObjectGUID guid);
		// That object's component of type componentID, or null
		GameObjectComponentPtr<GameObjectComponentBase> FindComponent(ObjectGUID guid, GameObjectComponentID componentID);
	}
}
