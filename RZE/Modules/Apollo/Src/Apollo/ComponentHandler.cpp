#include <Apollo/ComponentHandler.h>

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

	U32 ComponentHandler::TryResize()
	{
		if (mCapacity > mNextAvailEntityID)
		{
			return mCapacity;
		}

		U32 newCapacity = mCapacity * 2;
		return Resize(newCapacity);
	}

	U32 ComponentHandler::Resize(U32 newCapacity)
	{
		mEntities.resize(newCapacity);

		AssertExpr(mEntities.size() == newCapacity);
		return mEntities.size();
	}

}