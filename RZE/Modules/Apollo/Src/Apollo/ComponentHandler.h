#pragma once

#include <vector>

#include <Apollo/ECS/Entity.h>

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

	template <typename TComponent>
	bool HasComponent(EntityID entityID) const
	{
		const Entity& entity = GetEntity(entityID);
		return entity.mComponentSet[TComponent::GetTypeID()];
	}

	Entity& GetEntity(EntityID entityID);

	private:
		U32 mCapacity;
		U32 mSize;
		EntityID mNextAvailEntityID;

		EntityList mEntities;
	};
}