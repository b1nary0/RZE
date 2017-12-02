#include <Apollo/ComponentHandler.h>

namespace Apollo
{
	ComponentHandler::ComponentHandler()
	{
	}

	Entity& ComponentHandler::GetEntity(EntityID entityID)
	{
		return mEntities[entityID];
	}

}