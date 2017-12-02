#pragma once

#include <unordered_map>
#include <vector>

#include <Apollo/ECS/Entity.h>
#include <Apollo/ECS/EntityComponent.h>

#include <Utils/PrimitiveDefs.h>

namespace Apollo
{
	typedef U32 EntityID;

	class ComponentHandler
	{
		typedef std::vector<Entity> EntityList;
	public:
		ComponentHandler();

		EntityID CreateEntity();

		template <typename TComponentType, typename... TArgs>
		TComponentType& AddComponent(TArgs... args)
		{

		}

		template <typename TComponent>
		bool HasComponent(EntityID entityID) const
		{
			const Entity& entity = mEntities[entityID];
			return entity.mComponentSet[TComponent::GetID()];
		}

		Entity& GetEntity(EntityID entityID);

	private:
		void Initialize();

		U32 TryResize();
		U32 Resize(U32 newCapacity);

	private:
		U32 mCapacity;
		U32 mSize;
		EntityID mNextAvailEntityID;

		EntityList mEntities;
	};
}