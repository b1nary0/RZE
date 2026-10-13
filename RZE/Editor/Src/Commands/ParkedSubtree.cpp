#include <Commands/ParkedSubtree.h>

#include <UI/Framework/EditorSelection.h>

#include <RZE.h>

#include <Game/GameScene.h>
#include <Game/World/GameObject/GameObject.h>

#include <Utils/DebugUtils/Debug.h>

#include <algorithm>
#include <vector>

namespace Editor
{
	ParkedSubtree::ParkedSubtree(GameObjectPtr root)
		: m_root(root)
	{
		AssertExpr(m_root != nullptr && m_root->IsInScene());
	}

	void ParkedSubtree::Park(GameScene& scene, EditorSelection& selection)
	{
		AssertExpr(m_root != nullptr && m_root->IsInScene());
		if (m_root == nullptr || !m_root->IsInScene())
		{
			return;
		}

		std::vector<GameObject*> subtree;
		m_root->GetSubtree(subtree);

		const GameObjectPtr mainCamera = scene.GetMainCamera();
		m_mainCameraGUID = mainCamera != nullptr && std::find(subtree.begin(), subtree.end(), *mainCamera) != subtree.end()
			? mainCamera->GetGUID() : ObjectGUIDs::k_invalid;

		// While the objects are still in the scene, so whatever reacts to the selection (e.g. SelectionBoundsDisplay)
		// can still reach them
		std::vector<GameObjectPtr> remaining;
		for (const GameObjectPtr& selected : selection.GetAll())
		{
			if (std::find(subtree.begin(), subtree.end(), *selected) == subtree.end())
			{
				remaining.push_back(selected);
			}
		}
		selection.SetSelection(remaining);

		const GameObjectPtr parent = m_root->GetParent();
		m_parentGUID = parent != nullptr ? parent->GetGUID() : ObjectGUIDs::k_invalid;
		m_childIndex = parent != nullptr ? parent->GetChildIndex(*m_root) : -1;
		if (parent != nullptr)
		{
			m_root->DetachFromParent();
		}

		for (GameObject* gameObject : subtree)
		{
			gameObject->OnRemoveFromScene();
		}
	}

	void ParkedSubtree::Restore(GameScene& scene)
	{
		AssertExpr(IsParked());
		if (!IsParked())
		{
			return;
		}

		if (m_parentGUID != ObjectGUIDs::k_invalid)
		{
			// Undo and redo run in order, so the parent is back by now
			const GameObjectPtr parent = scene.FindGameObjectByGUID(m_parentGUID);
			AssertMsg(parent != nullptr, "A parked object's parent isn't in the scene to put it back under");
			if (parent != nullptr)
			{
				m_root->AttachTo(parent, m_childIndex);
			}
		}

		std::vector<GameObject*> subtree;
		m_root->GetSubtree(subtree);
		for (GameObject* gameObject : subtree)
		{
			gameObject->OnAddToScene();
		}

		if (m_mainCameraGUID != ObjectGUIDs::k_invalid)
		{
			// Takes it back from whichever camera the scene picked meanwhile
			scene.SetMainCamera(scene.FindGameObjectByGUID(m_mainCameraGUID));
		}
	}

	bool ParkedSubtree::IsParked() const
	{
		return m_root != nullptr && !m_root->IsInScene();
	}

	void ParkedSubtree::DestroyIfParked(GameScene& scene)
	{
		if (IsParked())
		{
			scene.RemoveGameObject(m_root);
		}
	}
}
