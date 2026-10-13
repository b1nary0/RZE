#include <Services/GameObjectClipboard.h>

#include <Game/World/GameObject/GameObject.h>
#include <Game/World/GameObjectComponents/EditorCameraComponent.h>

#include <algorithm>

namespace Editor
{
	void GameObjectClipboard::Copy(const std::vector<GameObjectPtr>& gameObjects)
	{
		std::vector<GameObjectSnapshot> contents = TakeSnapshots(gameObjects);
		// Keep what was there if nothing could be copied, rather than clearing it
		if (!contents.empty())
		{
			m_contents = std::move(contents);
			m_contentsWidth = MeasureWidth(gameObjects);
			m_pasteCount = 0;
		}
	}

	float GameObjectClipboard::MeasureWidth(const std::vector<GameObjectPtr>& gameObjects)
	{
		bool hasBounds = false;
		float minX = 0.0f;
		float maxX = 0.0f;
		for (const GameObjectPtr& gameObject : gameObjects)
		{
			Vector3D boundsMin;
			Vector3D boundsMax;
			if (gameObject == nullptr || !gameObject->GetHierarchyBounds(boundsMin, boundsMax))
			{
				continue;
			}

			minX = hasBounds ? std::min(minX, boundsMin.X()) : boundsMin.X();
			maxX = hasBounds ? std::max(maxX, boundsMax.X()) : boundsMax.X();
			hasBounds = true;
		}

		return hasBounds ? maxX - minX : 0.0f;
	}

	std::vector<GameObjectSnapshot> GameObjectClipboard::TakeSnapshots(const std::vector<GameObjectPtr>& gameObjects)
	{
		std::vector<GameObjectSnapshot> snapshots;
		for (const GameObjectPtr& gameObject : gameObjects)
		{
			// The editor camera is the editor's own, not part of the scene. It isn't selectable, but a copy would be a
			// second one.
			if (gameObject == nullptr || gameObject->GetComponent<EditorCameraComponent>() != nullptr)
			{
				continue;
			}

			const bool hasCopiedAncestor = std::any_of(gameObjects.begin(), gameObjects.end(),
				[&gameObject](const GameObjectPtr& other)
				{
					return other != gameObject && other != nullptr && other->IsSelfOrAncestorOf(gameObject);
				});
			if (!hasCopiedAncestor)
			{
				snapshots.push_back(GameObjectSnapshot::Take(**gameObject));
			}
		}

		return snapshots;
	}
}
