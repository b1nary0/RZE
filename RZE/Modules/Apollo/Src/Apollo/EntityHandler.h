#pragma once

#include <unordered_map>
#include <queue>
#include <vector>

#include <Apollo/ECS/Entity.h>
#include <Apollo/ECS/EntityComponent.h>
#include <Apollo/ECS/EntitySystem.h>

#include <Utils/PrimitiveDefs.h>
#include <Utils/Functor.h>

#define APOLLO_REGISTER_COMPONENT(ComponentType)											\
{																							\
	Apollo::ComponentTypeID<Apollo::ComponentBase>::GetComponentTypeID<ComponentType>();	\
	Apollo::EntityHandler::RegisterComponentType(ComponentType::GetID(), #ComponentType);							\
	Apollo::EntityHandler::AddComponentFactory(ComponentType::GetID(), Functor<Apollo::ComponentBase*>([]() { return new ComponentType();}));\
}																							\

namespace Apollo
{
	const U32 kInvalidEntityID = 0;

	class EntityHandler
	{
	private:
		struct ComponentIDQueueData
		{
			ComponentIDQueueData(ComponentID componentID, EntityID entityID)
				: mComponentID(componentID)
				, mEntityID(entityID) {}

			ComponentID mComponentID;
			EntityID	mEntityID;
		};

	public:
		typedef Functor<void, EntityID> ComponentAddedFunc;
		typedef Functor<void, EntityID> ComponentRemovedFunc;
		typedef Functor<void, EntityID> ComponentModifiedFunc;

		typedef std::vector<Entity>														EntityList;
		typedef std::vector<EntityID>													EntityFreeList;
		typedef std::vector<ComponentBase*>												ComponentList;
		typedef std::vector<EntitySystem*>												SystemList;
		typedef std::unordered_map<ComponentID, std::string>							ComponentNameIDMap;
		typedef std::unordered_map<EntityID, ComponentList>								EntityComponentMapping;
		typedef std::unordered_map<ComponentID, std::vector<ComponentAddedFunc>>		OnComponentAddedMap;
		typedef std::unordered_map <ComponentID, std::vector<ComponentRemovedFunc>>		OnComponentRemovedMap;
		typedef std::unordered_map <ComponentID, std::vector<ComponentModifiedFunc>>	OnComponentModifiedMap;
		typedef std::queue<ComponentIDQueueData>										ComponentIDQueue;
		typedef std::unordered_map<ComponentID, Functor<ComponentBase*>>				ComponentFactoryMap;

	public:
		EntityHandler();

	public:
		void Initialize();
		void PreUpdate();
		void Update();
		void ShutDown();

	public:
		template <typename TComponentType>
		void RegisterForComponentAddNotification(ComponentAddedFunc callback);

		template <typename TComponentType>
		void RegisterForComponentRemovedNotification(ComponentRemovedFunc callback);

		template <typename TComponenType>
		void RegisterForComponentModifiedNotification(ComponentModifiedFunc callback);

	public:
		// #TODO(Josh) I don't like this, should have a better place for this type of behaviour/necessity
		static void RegisterComponentType(ComponentID componentID, const std::string& componentName);
		static void AddComponentFactory(ComponentID componentID, const Functor<ComponentBase*>& factoryFunc);
		static const ComponentNameIDMap& GetAllComponentTypes();

	public:
		EntityID CreateEntity(const std::string& name);

		void DestroyEntity(EntityID entityID);

		Entity& GetEntity(EntityID entityID);

		template <typename TComponentType, typename... TArgs>
		TComponentType* AddComponent(EntityID entityID, TArgs... args);
		
		template <typename TComponentType>
		TComponentType* GetComponent(EntityID entityID);

		template <typename TComponent>
		bool HasComponent(EntityID entityID) const;

		template <typename TComponent>
		void OnComponentModified(EntityID entityID);

		template <typename TSystemType, typename... TArgs>
		TSystemType* AddSystem(TArgs... args);

		// Look over these and maybe have a better grouping solution for components
		template <typename TComponent>
		void ForEach(Functor<void, EntityID> callback);

		template <typename TComponent0, typename TComponent1>
		void ForEach(Functor<void, EntityID> callback);

	public:
		void GetAllComponents(EntityID entityID, ComponentList& outComponents);
		void GetComponentNames(EntityID entityID, ComponentNameIDMap& outComponentNames);
		ComponentID GetComponentIDFromTypeName(const std::string& typeNameStr);
		ComponentBase* AddComponentByID(EntityID entityID, ComponentID componentID);
		void RemoveComponent(EntityID entityID, ComponentID componentID);

	private:
		U32 TryResize();
		U32 Resize(U32 newCapacity);

		void ResetEntity(EntityID newID);
		void FlushComponentIDQueues();

	private:
		static ComponentNameIDMap sComponentTypeRegistry;
		static ComponentFactoryMap sComponentFactories;

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
		OnComponentModifiedMap mOnComponentModifiedMap;
		ComponentIDQueue mComponentsAddedThisFrame;
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

	template <typename TComponent>
	void EntityHandler::OnComponentModified(EntityID entityID)
	{
		auto& it = mOnComponentModifiedMap.find(TComponent::GetID());
		if (it != mOnComponentModifiedMap.end())
		{
			for (auto& func : (*it).second)
			{
				func(entityID);
			}
		}
	}

	template <typename TSystemType, typename... TArgs>
	TSystemType* EntityHandler::AddSystem(TArgs... args)
	{
		TSystemType* const system = new TSystemType(this, std::forward<TArgs>(args)...);
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

	template <typename TComponentType>
	void EntityHandler::RegisterForComponentModifiedNotification(ComponentModifiedFunc callback)
	{
		ComponentID componentID = TComponentType::GetID();
		mOnComponentModifiedMap[componentID].push_back(callback);
	}

	template <typename TComponentType, typename... TArgs>
	TComponentType* EntityHandler::AddComponent(EntityID entityID, TArgs... args)
	{
		ComponentID componentID = TComponentType::GetID();

		//#TODO(Josh) Not sure how I feel about this
		if (HasComponent<TComponentType>(entityID))
		{
			RemoveComponent(entityID, componentID);
		}

		TComponentType* const newComp = new TComponentType(std::forward<TArgs>(args)...);
		mEntityComponentMap[entityID][componentID] = newComp;

		mEntities[entityID].mComponentSet[componentID] = true;

		mComponentsAddedThisFrame.emplace(componentID, entityID);

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