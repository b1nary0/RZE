#pragma once

#include <unordered_map>
#include <vector>

#include <Apollo/ECS/Entity.h>
#include <Apollo/ECS/EntityComponent.h>
#include <Apollo/ECS/EntitySystem.h>

#include <Utils/PrimitiveDefs.h>
#include <Utils/Functor.h>

namespace Apollo
{
	typedef U32 EntityID;

	class ComponentHandler
	{
	public:
		typedef Functor<void, EntityID, ComponentHandler&> ComponentAddedFunc;

		typedef std::vector<Entity> EntityList;
		typedef std::vector<ComponentBase*> ComponentList;
		typedef std::vector<EntitySystem*> SystemList;
		typedef std::unordered_map<EntityID, ComponentList> EntityComponentMapping;
		typedef std::unordered_map<ComponentID, std::vector<ComponentAddedFunc>> OnComponentAddedMap;

	public:
		ComponentHandler();

	public:
		void Initialize();
		void Update();
		void ShutDown();

		template <typename TComponentType>
		void RegisterForComponentAddNotification(ComponentAddedFunc callback);

	public:
		EntityID CreateEntity();
		Entity& GetEntity(EntityID entityID);

		template <typename TComponentType, typename... TArgs>
		TComponentType* AddComponent(EntityID entityID, TArgs... args);

		template <typename TComponentType>
		TComponentType* GetComponent(EntityID entityID);

		template <typename TComponent>
		bool HasComponent(EntityID entityID) const;

		template <typename TSystemType, typename... TArgs>
		TSystemType* AddSystem(TArgs... args);

	private:
		U32 TryResize();
		U32 Resize(U32 newCapacity);

	private:
		U32 mCapacity;
		U32 mSize;
		EntityID mNextAvailEntityID;

		EntityList mEntities;
		EntityComponentMapping mEntityComponentMap;
		SystemList mSystems;

		OnComponentAddedMap mOnComponentAddedMap;
	};

	template <typename TComponent>
	bool ComponentHandler::HasComponent(EntityID entityID) const
	{
		if (entityID >= mCapacity)
		{
			return false;
		}

		const Entity& entity = mEntities[entityID];
		return entity.mComponentSet[TComponent::GetID()];
	}

	template <typename TSystemType, typename... TArgs>
	TSystemType* ComponentHandler::AddSystem(TArgs... args)
	{
		TSystemType* const system = new TSystemType(std::forward<TArgs>(args)...);
		mSystems.push_back(system);
		system->Initialize();
		return system;
	}

	template <typename TComponentType>
	void ComponentHandler::RegisterForComponentAddNotification(ComponentAddedFunc callback)
	{
		ComponentID componentID = TComponentType::GetID();
		mOnComponentAddedMap[componentID].push_back(callback);
	}

	template <typename TComponentType, typename... TArgs>
	TComponentType* ComponentHandler::AddComponent(EntityID entityID, TArgs... args)
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
	TComponentType* ComponentHandler::GetComponent(EntityID entityID)
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