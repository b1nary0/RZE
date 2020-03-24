#include <Apollo/EntityHandler.h>

#include <Optick/optick.h>

#include <Utils/Platform/Timers/HiResTimer.h>

namespace Apollo
{
	EntityHandler::ComponentNameIDMap EntityHandler::sComponentTypeRegistry;
	EntityHandler::ComponentFactoryMap EntityHandler::sComponentFactories;

	EntityHandler::EntityHandler()
	{
		mNextAvailEntityID = 1;
		mCapacity = 0;
		mSize = 0;

		Initialize();
	}

	void EntityHandler::RegisterComponentType(ComponentID componentID, const std::string& componentName)
	{
		sComponentTypeRegistry.insert({componentID, componentName});
	}

	void EntityHandler::AddComponentFactory(ComponentID componentID, const Functor<ComponentBase*>& factoryFunc)
	{
		sComponentFactories.insert({componentID, factoryFunc});
	}

	const Apollo::EntityHandler::ComponentNameIDMap& EntityHandler::GetAllComponentTypes()
	{
		return sComponentTypeRegistry;
	}

	EntityID EntityHandler::CreateEntity(const std::string& name)
	{
		if (!mEntityFreeList.empty())
		{
			EntityID id = mEntityFreeList.back();
			mEntityFreeList.pop_back();

			ResetEntity(id);

			return id;
		}

		TryResize();
		EntityID id = mNextAvailEntityID++;
		ResetEntity(id);

		return id;
	}

	void EntityHandler::DestroyEntity(EntityID entityID)
	{
		ComponentList& components = mEntityComponentMap[entityID];
		for (auto& component : components)
		{
			if (!component)
			{
				continue;
			}

			RemoveComponent(entityID, component->id);

			delete component;
			component = nullptr;
		}

		mEntityFreeList.push_back(entityID);
		ResetEntity(entityID);
	}

	Entity& EntityHandler::GetEntity(EntityID entityID)
	{
		return mEntities[entityID];
	}

	Apollo::ComponentBase* EntityHandler::AddComponentByID(EntityID entityID, ComponentID componentID)
	{
		ComponentBase* const newComp = sComponentFactories.at(componentID)();
		mEntityComponentMap[entityID][componentID] = newComp;

		mEntities[entityID].mComponentSet[componentID] = true;

		mComponentsAddedThisFrame.emplace(componentID, entityID);

		return newComp;
	}

	void EntityHandler::RemoveComponent(EntityID entityID, ComponentID componentID)
	{
		Apollo::ComponentBase* const component = mEntityComponentMap[entityID][componentID];
		AssertNotNull(component);

		// This is done pre-delete so the things that want notification can access the data if needed.
		// Multithreading may(will?) cause issues for this in the future I believe.
		auto& it = mOnComponentRemovedMap.find(componentID);
		if (it != mOnComponentRemovedMap.end())
		{
			for (auto& func : (*it).second)
			{
				func(entityID);
			}
		}

		mEntities[entityID].mComponentSet[componentID] = false;
		delete component;
		mEntityComponentMap[entityID][componentID] = nullptr;
	}

	void EntityHandler::GetAllComponents(EntityID entityID, ComponentList& outComponents)
	{
		for (ComponentBase* component : mEntityComponentMap[entityID])
		{
			// #NOTE(A quirk of how we store component data)
			if (component != nullptr)
			{
				outComponents.push_back(component);
			}
		}
	}

	void EntityHandler::GetComponentNames(EntityID entityID, ComponentNameIDMap& outComponentNames)
	{
		const ComponentList& components = mEntityComponentMap[entityID];
		outComponentNames.reserve(components.size());
		for (auto& component : components)
		{
			if (component != nullptr)
			{
				outComponentNames.emplace(component->id, component->ComponentName);
			}
		}
	}

	Apollo::ComponentID EntityHandler::GetComponentIDFromTypeName(const std::string& typeNameStr)
	{
		// #TODO(Josh::Map iteration, not nice)
		for (auto& pair : sComponentTypeRegistry)
		{
			if (pair.second == typeNameStr)
			{
				return pair.first;
			}
		}

		AssertFalse();
		return 0;
	}

	void EntityHandler::Initialize()
	{
		mCapacity = Resize(32);
	}

	void EntityHandler::Update()
	{	
		OPTICK_EVENT();

		FlushComponentIDQueues();

		for (size_t idx = 0; idx < mSystems.size(); ++idx)
		{
			EntitySystem* system = mSystems[idx];
			const EntityComponentFilter& filter = system->GetComponentFilter();

			std::vector<EntityID> filteredEntities;
			filteredEntities.reserve(mEntities.size());
			filter.FilterAtLeast(mEntities, filteredEntities);

			if (!filteredEntities.empty())
			{
				system->Update(filteredEntities);
			}
		}
	}

	void EntityHandler::ShutDown()
	{

	}

	U32 EntityHandler::TryResize()
	{
		if (mCapacity > mNextAvailEntityID)
		{
			return mCapacity;
		}

		U32 newCapacity = mCapacity + mCapacity / 2;
		return Resize(newCapacity);
	}

	U32 EntityHandler::Resize(U32 newCapacity)
	{
		mEntities.resize(newCapacity);

		AssertExpr(mEntities.size() == newCapacity);
		mCapacity = static_cast<U32>(mEntities.size());
		return mCapacity;
	}

	void EntityHandler::ResetEntity(EntityID newID)
	{
		Entity& entity = mEntities[newID];
		entity.mEntityID = newID;
		entity.mComponentSet.reset();

		mEntityComponentMap[newID].resize(ENTITY_MAX_COMPONENTS);
	}

	void EntityHandler::FlushComponentIDQueues()
	{
		while (!mComponentsAddedThisFrame.empty())
		{
			const ComponentIDQueueData& data = mComponentsAddedThisFrame.front();
			mComponentsAddedThisFrame.pop();

			auto& it = mOnComponentAddedMap.find(data.mComponentID);
			if (it != mOnComponentAddedMap.end())
			{
				for (auto& func : (*it).second)
				{
					func(data.mEntityID);
				}
			}
		}
	}

}