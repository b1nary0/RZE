#include <StdAfx.h>
#include <Game/World/GameObject/EditorGameComponentInfoCache.h>

namespace EditorComponentCache
{
	ComponentOrderMap s_componentTypeRegistry;

	void RegisterComponentType(GameObjectComponentID id, uint32_t componentOrder )
	{
		s_componentTypeRegistry[id].Order = componentOrder;
	}

	const ComponentOrderMap& GetAllComponentReflectData()
	{
		return s_componentTypeRegistry;
	}

}