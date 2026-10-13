#include <Commands/UndoRecord.h>

#include <RZE.h>

#include <Game/World/GameObject/GameObject.h>

#include <vector>

namespace Editor
{
	namespace UndoTargets
	{
		GameObjectPtr FindObject(ObjectGUID guid)
		{
			return RZE().GetActiveScene().FindGameObjectByGUID(guid);
		}

		bool IsInScene(const GameObjectPtr& gameObject)
		{
			return gameObject != nullptr && gameObject->IsInScene();
		}

		GameObjectComponentPtr<GameObjectComponentBase> FindComponent(ObjectGUID guid, GameObjectComponentID componentID)
		{
			const GameObjectPtr gameObject = FindObject(guid);
			if (gameObject == nullptr)
			{
				return nullptr;
			}

			std::vector<GameObjectComponentPtr<GameObjectComponentBase>> components;
			gameObject->GetComponentHandles(components);
			for (const GameObjectComponentPtr<GameObjectComponentBase>& component : components)
			{
				if (component->m_id == componentID)
				{
					return component;
				}
			}
			return nullptr;
		}
	}
}
