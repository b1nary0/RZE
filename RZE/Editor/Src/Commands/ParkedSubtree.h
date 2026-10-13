#pragma once

#include <Game/World/GameObject/GameObjectDefinitions.h>
#include <Game/World/GameObject/ObjectGUID.h>

class GameScene;

namespace Editor
{
	class EditorSelection;

	// A GameObject and its descendants, which undo takes out of the scene and puts back as the same objects, rather
	// than destroying and rebuilding them. Parked objects stay alive and owned by the scene, in their place in its
	// list (so root order and GUIDs come back unchanged), but are out of it: lookups, saving and updates skip them.
	//
	// Handles to a parked object stay valid, so code holding one checks UndoTargets::IsInScene before changing it.
	class ParkedSubtree
	{
	public:
		// root must be in the scene; it isn't parked until Park
		explicit ParkedSubtree(GameObjectPtr root);

	public:
		// Takes the subtree out of the scene and out of the selection, remembering where it was in its parent
		void Park(GameScene& scene, EditorSelection& selection);
		// Puts it back where it was, and gives the main camera role back if it was in the subtree when parked
		void Restore(GameScene& scene);

		bool IsParked() const;
		// For UndoRecord::OnDropped: destroys the objects if they're parked, since nothing will put them back
		void DestroyIfParked(GameScene& scene);

		const GameObjectPtr& GetRoot() const { return m_root; }

	private:
		GameObjectPtr m_root;
		ObjectGUID m_parentGUID = ObjectGUIDs::k_invalid;
		int m_childIndex = -1;
		// The main camera, if it was in the subtree when it was parked; the scene picks another one meanwhile
		ObjectGUID m_mainCameraGUID = ObjectGUIDs::k_invalid;
	};
}
