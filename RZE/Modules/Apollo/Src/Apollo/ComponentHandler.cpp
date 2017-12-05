#include <Apollo/ComponentHandler.h>

#include <Utils/Platform/Timers/HiResTimer.h>

namespace Apollo
{
	ComponentHandler::ComponentHandler()
	{
		mNextAvailEntityID = 0;
		mCapacity = 0;
		mSize = 0;

		Initialize();
	}

	EntityID ComponentHandler::CreateEntity()
	{
		TryResize();

		EntityID id = mNextAvailEntityID++;
		Entity& entity = mEntities[id];

		entity.mEntityID = id;
		entity.mComponentSet.reset();

		mEntityComponentMap[id].resize(ENTITY_MAX_COMPONENTS);

		return id;
	}

	Entity& ComponentHandler::GetEntity(EntityID entityID)
	{
		return mEntities[entityID];
	}

	void ComponentHandler::Initialize()
	{
		mCapacity = Resize(32);
	}

	void ComponentHandler::Update()
	{
		//HiResTimer timer;
		//timer.Start();
		for (size_t idx = 0; idx < mSystems.size(); ++idx)
		{
			EntitySystem* system = mSystems[idx];
			
			const EntityComponentFilter& filter = system->GetComponentFilter();
			std::vector<EntityID> filteredEntities;

			filter.FilterEachOf(mEntities, filteredEntities);

			system->Update(filteredEntities);
		}
		//timer.Stop();
		
// 		static bool test = true;
// 		static int next = 0;
// 		if (++next > 25) test = true;
// 		if (test)
// 		{
// 			LOG_CONSOLE_ARGS("Filter took %f ms on %i entities.", timer.GetElapsed<float>() * 1000.0f, static_cast<int>(mEntities.size()));
// 			test = false;
// 			next = 0;
// 		}
	}

	void ComponentHandler::ShutDown()
	{

	}

	U32 ComponentHandler::TryResize()
	{
		if (mCapacity > mNextAvailEntityID)
		{
			return mCapacity;
		}

		U32 newCapacity = mCapacity + mCapacity / 2;
		return Resize(newCapacity);
	}

	U32 ComponentHandler::Resize(U32 newCapacity)
	{
		mEntities.resize(newCapacity);

		AssertExpr(mEntities.size() == newCapacity);
		mCapacity = mEntities.size();
		return mCapacity;
	}

}