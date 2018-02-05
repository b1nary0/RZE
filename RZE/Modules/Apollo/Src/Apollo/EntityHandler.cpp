#include <Apollo/EntityHandler.h>

#include <Utils/Platform/Timers/HiResTimer.h>

namespace Apollo
{
	EntityHandler::EntityHandler()
	{
		mNextAvailEntityID = 0;
		mCapacity = 0;
		mSize = 0;

		Initialize();
	}

	EntityID EntityHandler::CreateEntity()
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
		ComponentList components = mEntityComponentMap[entityID];
		for (auto& component : components)
		{
			delete component;
		}

		mEntityFreeList.push_back(entityID);
	}

	Entity& EntityHandler::GetEntity(EntityID entityID)
	{
		return mEntities[entityID];
	}

	void EntityHandler::Initialize()
	{
		mCapacity = Resize(32);
	}

	void EntityHandler::Update()
	{
		for (size_t idx = 0; idx < mSystems.size(); ++idx)
		{
			EntitySystem* system = mSystems[idx];
			
			const EntityComponentFilter& filter = system->GetComponentFilter();
			std::vector<EntityID> filteredEntities;

			filter.FilterAtLeast(mEntities, filteredEntities);

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

}