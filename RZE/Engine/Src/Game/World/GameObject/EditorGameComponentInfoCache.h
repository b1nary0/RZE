#pragma once

#include <Game/World/GameObject/GameObjectDefinitions.h>

#include <Utils/Functor.h>

#include <RapidJSON/document.h>
#include <RapidJSON/prettywriter.h>
#include <RapidJSON/stringbuffer.h>

#include <string>

#define REGISTER_EDITORCOMPONENTCACHE_ORDERDATA(ComponentType, ComponentOrder)\
{\
	GameObjectComponentTypeID<GameObjectComponentBase>::GetComponentTypeID<ComponentType>();	\
	EditorComponentCache::RegisterComponentType(ComponentType::GetID(), ComponentOrder);\
}

class GameObject;
class GameObjectComponentBase;

// #TODO Would ideally like to come back to this setup
namespace EditorComponentCache
{
	struct EditorComponentInfo
	{
		uint16_t Order = 0x0010;
	};
	typedef std::unordered_map<GameObjectComponentID, EditorComponentInfo> ComponentInfoMap;

	void RegisterComponentType(GameObjectComponentID id, uint32_t componentOrder);

	const ComponentInfoMap& GetAllComponentsInfo();
}