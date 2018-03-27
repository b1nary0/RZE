#pragma once

#include <unordered_map>
#include <vector>

#include <Apollo/ECS/Entity.h>
#include <Apollo/ECS/EntityComponent.h>
#include <Apollo/ECS/EntitySystem.h>

#include <Utils/PrimitiveDefs.h>
#include <Utils/Functor.h>

#define APOLLO_REGISTER_COMPONENT(ComponentType)											\
{																							\
	Apollo::ComponentTypeID<Apollo::ComponentBase>::GetComponentTypeID<ComponentType>();	\
}																							\

namespace Apollo
{
	typedef U32 EntityID;

	class EntityHandler
	{
	public:
		typedef Functor<void, EntityID, EntityHandler&> ComponentAddedFunc;
		typedef Functor<void, EntityID, EntityHandler&> ComponentRemovedFunc;

		typedef std::vector<Entity> EntityList;
		typedef std::vector<EntityID> EntityFreeList;
		typedef std::vector<ComponentBase*> ComponentList;
		typedef std::vector<EntitySystem*> SystemList;
		typedef std::unordered_map<EntityID, ComponentList> EntityComponentMapping;
		typedef std::unordered_map<ComponentID, std::vector<ComponentAddedFunc>> OnComponentAddedMap;
		typedef std::unordered_map <ComponentID, std::vector<ComponentRemovedFunc>> OnComponentRemovedMap;

	public:
		EntityHandler();

	public:
		void Initialize();
		void Update();
		void ShutDown();

	public:
		template <typename TComponentType>
		void RegisterForComponentAddNotification(ComponentAddedFunc callback);

		template <typename TComponentType>
		void RegisterForComponentRemovedNotification(ComponentRemovedFunc callback);

	public:
		EntityID CreateEntity();

		void DestroyEntity(EntityID entityID);

		Entity& GetEntity(EntityID entityID);

		template <typename TComponentType, typename... TArgs>
		TComponentType* AddComponent(EntityID entityID, TArgs... args);

		void RemoveComponent(EntityID entityID, ComponentID componentID);

		template <typename TComponentType>
		TComponentType* GetComponent(EntityID entityID);

		template <typename TComponent>
		bool HasComponent(EntityID entityID) const;

		template <typename TSystemType, typename... TArgs>
		TSystemType* AddSystem(TArgs... args);

	public:
		// Look over these and maybe have a better grouping solution for components
		template <typename TComponent>
		void ForEach(Functor<void, EntityID> callback);

		template <typename TComponent0, typename TComponent1>
		void ForEach(Functor<void, EntityID> callback);

	private:
		U32 TryResize();
		U32 Resize(U32 newCapacity);

		void ResetEntity(EntityID newID);

	private:
		U32 mCapacity;
		U32 mSize;
		EntityID mNextAvailEntityID;

		EntityList mEntities;
		EntityFreeList mEntityFreeList;
		EntityComponentMapping mEntityComponentMap;
		SystemList mSystems;

		OnComponentAddedMap mOnComponentAddedMap;
		OnComponentRemovedMap mOnComponentRemovedMap;
	};

	template <typename TComponent>
	bool EntityHandler::HasComponent(EntityID entityID) const
	{
		if (entityID >= mCapacity)
		{
			return false;
		}

		const Entity& entity = mEntities[entityID];
		return entity.mComponentSet[TComponent::GetID()];
	}

	template <typename TSystemType, typename... TArgs>
	TSystemType* EntityHandler::AddSystem(TArgs... args)
	{
		TSystemType* const system = new TSystemType(std::forward<TArgs>(args)...);
		mSystems.push_back(system);
		system->Initialize();
		return system;
	}

	template <typename TComponent>
	void EntityHandler::ForEach(Functor<void, EntityID> callback)
	{
		std::bitset<ENTITY_MAX_COMPONENTS> componentSet;
		componentSet[TComponent::GetID()] = true;

		for (auto& entity : mEntities)
		{
			std::bitset<ENTITY_MAX_COMPONENTS> filtered = entity.mComponentSet & componentSet;
			if (filtered.any())
			{
				callback(entity.mEntityID);
			}
		}
	}

	template <typename TComponent0, typename TComponent1>
	void EntityHandler::ForEach(Functor<void, EntityID> callback)
	{
		std::bitset<ENTITY_MAX_COMPONENTS> componentSet;
		componentSet[TComponent0::GetID()] = true;
		componentSet[TComponent1::GetID()] = true;

		for (auto& entity : mEntities)
		{
			std::bitset<ENTITY_MAX_COMPONENTS> filtered = entity.mComponentSet & componentSet;
			if (filtered == componentSet)
			{
				callback(entity.mEntityID);
			}
		}
	}

	template <typename TComponentType>
	void EntityHandler::RegisterForComponentAddNotification(ComponentAddedFunc callback)
	{
		ComponentID componentID = TComponentType::GetID();
		mOnComponentAddedMap[componentID].push_back(callback);
	}

	template <typename TComponentType>
	void EntityHandler::RegisterForComponentRemovedNotification(ComponentRemovedFunc callback)
	{
		ComponentID componentID = TComponentType::GetID();
		mOnComponentRemovedMap[componentID].push_back(callback);
	}

	template <typename TComponentType, typename... TArgs>
	TComponentType* EntityHandler::AddComponent(EntityID entityID, TArgs... args)
	{
		if (HasComponent<TComponentType>(entityID))
		{
			return nullptr;
		}

		ComponentID componentID = TComponentType::GetID();

		TComponentType* const newComp = new TComponentType(std::forward<TArgs>(args)...);
		mEntityComponentMap[entityID][componentID] = newComp;

		mEntities[entityID].mComponentSet[componentID] = true;

		if (mOnComponentAddedMap.count(componentID))
		{
			for (auto& func : mOnComponentAddedMap[componentID])
			{
				func(entityID, *this);
			}
		}

		return newComp;
	}

	template <typename TComponentType>
	TComponentType* EntityHandler::GetComponent(EntityID entityID)
	{
		if (!HasComponent<TComponentType>(entityID))
		{
			return nullptr;
		}

		ComponentID componentID = TComponentType::GetID();
		TComponentType* const component = static_cast<TComponentType*>(mEntityComponentMap[entityID][componentID]);
		AssertNotNull(component && "We should not have passed the above HasComponent() check and recieved a null component");

		return component;
	}

}