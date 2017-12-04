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
		typedef std::vector<ComponentBase*> ComponentList;
		typedef std::unordered_map<EntityID, ComponentList> EntityComponentMapping;
	public:
		ComponentHandler();

		EntityID CreateEntity();

		template <typename TComponentType, typename... TArgs>
		TComponentType* AddComponent(EntityID entityID, TArgs... args);

		template <typename TComponent>
		bool HasComponent(EntityID entityID) const;

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
		EntityComponentMapping mEntityComponentMap;
	};

	template <typename TComponent>
	bool ComponentHandler::HasComponent(EntityID entityID) const
	{
		const Entity& entity = mEntities[entityID];
		return entity.mComponentSet[TComponent::GetID()];
	}

	template <typename TComponentType, typename... TArgs>
	TComponentType* ComponentHandler::AddComponent(EntityID entityID, TArgs... args)
	{
		if (HasComponent<TComponentType>(entityID))
		{
			return nullptr;
		}

		U32 componentID = TComponentType::GetID();

		TComponentType* newComp = new TComponentType(std::forward<TArgs>(args)...);
		mEntityComponentMap[entityID][componentID] = newComp;

		mEntities[entityID].mComponentSet[componentID] = true;

		return newComp;
	}

}