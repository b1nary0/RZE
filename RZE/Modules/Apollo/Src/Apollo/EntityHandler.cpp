#include <Apollo/EntityHandler.h>

#include <Brofiler/Brofiler.h>

#include <Utils/Platform/Timers/HiResTimer.h>

namespace Apollo
{
	EntityHandler::ComponentNameList EntityHandler::sComponentNameRegistry;

	EntityHandler::EntityHandler()
	{
		mNextAvailEntityID = 0;
		mCapacity = 0;
		mSize = 0;

		Initialize();
	}

	void EntityHandler::RegisterComponentName(const std::string& componentName)
	{
		auto& iter = std::find(sComponentNameRegistry.begin(), sComponentNameRegistry.end(), componentName);
		if (iter == sComponentNameRegistry.end())
		{
			sComponentNameRegistry.emplace_back(componentName);
		}
	}

	const Apollo::EntityHandler::ComponentNameList& EntityHandler::GetAllComponentNames()
	{
		return sComponentNameRegistry;
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
		delete mEntityComponentMap[entityID][componentID];
		mEntityComponentMap[entityID][componentID] = nullptr;
	}

	void EntityHandler::GetComponentNames(EntityID entityID, ComponentNameIDMap& outComponentNames)
	{
		const ComponentList& components = mEntityComponentMap[entityID];
		for (auto& component : components)
		{
			if (component != nullptr)
			{
				outComponentNames.emplace(component->id, component->ComponentName);
			}
		}
	}

	void EntityHandler::Initialize()
	{
		mCapacity = Resize(32);
	}

	void EntityHandler::Update()
	{	BROFILER_CATEGORY("EntityHandler::Update", Profiler::Color::BlueViolet)
		FlushComponentIDQueues();

		for (size_t idx = 0; idx < mSystems.size(); ++idx)
		{
			EntitySystem* system = mSystems[idx];

			const EntityComponentFilter& filter = system->GetComponentFilter();
			std::vector<EntityID> filteredEntities;

			{	BROFILER_CATEGORY("Filtering Entity Components", Profiler::Color::CornflowerBlue)
				filter.FilterAtLeast(mEntities, filteredEntities);
			}

			system->Update(filteredEntities);
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
		mCapacity = mEntities.size();
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