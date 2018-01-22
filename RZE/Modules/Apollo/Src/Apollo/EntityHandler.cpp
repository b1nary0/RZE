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
		TryResize();

		EntityID id = mNextAvailEntityID++;
		Entity& entity = mEntities[id];

		entity.mEntityID = id;
		entity.mComponentSet.reset();

		mEntityComponentMap[id].resize(ENTITY_MAX_COMPONENTS);

		return id;
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

}