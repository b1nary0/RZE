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
		BoundingBox bounds;
		for (const GameObjectPtr& gameObject : gameObjects)
		{
			if (gameObject != nullptr)
			{
				bounds.Grow(gameObject->GetHierarchyBounds());
			}
		}

		return bounds.IsEmpty() ? 0.0f : bounds.GetSize().X();
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
