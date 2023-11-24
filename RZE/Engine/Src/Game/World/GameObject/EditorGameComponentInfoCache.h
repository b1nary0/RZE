#pragma once

#include <Game/World/GameObject/GameObjectDefinitions.h>

#define REGISTER_EDITORCOMPONENTCACHE_ORDERDATA(ComponentType, ComponentOrder)\
{\
	GameObjectComponentTypeID<GameObjectComponentBase>::GetComponentTypeID<ComponentType>();	\
	EditorComponentCache::RegisterComponentOrder(ComponentType::GetID(), ComponentOrder);\
}

namespace EditorComponentCache
{
	static constexpr const uint32_t kDefaultSortingOrder = 0x0100;
	struct EditorComponentInfo
	{
		uint32_t Order = kDefaultSortingOrder;
	};
	typedef std::unordered_map<GameObjectComponentID, EditorComponentInfo> ComponentInfoMap;

	void RegisterComponentOrder(GameObjectComponentID id, uint32_t componentOrder);

	const ComponentInfoMap& GetAllComponentsInfo();
}