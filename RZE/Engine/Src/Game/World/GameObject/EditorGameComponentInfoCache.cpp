#include <StdAfx.h>
#include <Game/World/GameObject/EditorGameComponentInfoCache.h>

namespace EditorComponentCache
{
	ComponentInfoMap s_componentTypeRegistry;

	void RegisterComponentOrder(GameObjectComponentID id, uint32_t componentOrder )
	{
		s_componentTypeRegistry[id].Order = componentOrder;
	}

	const ComponentInfoMap& GetAllComponentsInfo()
	{
		return s_componentTypeRegistry;
	}
}