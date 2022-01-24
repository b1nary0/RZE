#include <StdAfx.h>
#include <Game/World/GameObject/GameObjectComponent.h>

namespace GameObjectComponentRegistry
{
	typedef std::unordered_map<GameObjectComponentID, Functor<GameObjectComponentBase*>> ComponentFactoryMap;

	ComponentNameIDMap s_componentTypeRegistry;
	ComponentFactoryMap s_componentFactories;

	void RegisterComponentType(GameObjectComponentID id, const char* componentName)
	{
		s_componentTypeRegistry.insert({ id, componentName });
	}

	void AddComponentFactory(GameObjectComponentID id, const Functor<GameObjectComponentBase*>& factoryFunc)
	{
		s_componentFactories.insert({ id, factoryFunc });
	}

	GameObjectComponentBase* CreateComponentByID(GameObjectComponentID id)
	{
		return s_componentFactories.at(id)();
	}

	const ComponentNameIDMap& GetAllComponentReflectData()
	{
		return s_componentTypeRegistry;
	}

}