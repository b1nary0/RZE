#include <StdAfx.h>
#include <Game/World/GameObject/GameObjectComponent.h>

#include <unordered_set>

namespace
{
	// Never reset, so instance IDs are never reused (see GameObjectLifetime)
	GameObjectComponentInstanceID s_nextComponentInstanceID = 0;

	std::unordered_set<GameObjectComponentInstanceID> s_aliveComponents;
}

namespace GameObjectLifetime
{
	bool IsComponentAlive(GameObjectComponentInstanceID id)
	{
		return s_aliveComponents.find(id) != s_aliveComponents.end();
	}

	void CheckComponentAccess(GameObjectComponentInstanceID id)
	{
		AssertMsg(IsComponentAlive(id), "GameObjectComponentPtr dereferenced after its component was destroyed");
	}
}

GameObjectComponentBase::GameObjectComponentBase()
	: m_owner(nullptr)
	, m_instanceID(s_nextComponentInstanceID++)
{
	s_aliveComponents.insert(m_instanceID);
}

GameObjectComponentBase::~GameObjectComponentBase()
{
	s_aliveComponents.erase(m_instanceID);
}

namespace GameObjectComponentRegistry
{
	typedef std::unordered_map<GameObjectComponentID, Functor<GameObjectComponentBase*>> ComponentFactoryMap;
	typedef std::unordered_map<GameObjectComponentID, ResourceDependencyGatherer> ResourceDependencyGathererMap;

	ComponentNameIDMap s_componentTypeRegistry;
	ComponentFactoryMap s_componentFactories;
	ResourceDependencyGathererMap s_resourceDependencyGatherers;

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

	void SetResourceDependencyGatherer(GameObjectComponentID id, ResourceDependencyGatherer gatherer)
	{
		s_resourceDependencyGatherers[id] = gatherer;
	}

	ResourceDependencyGatherer GetResourceDependencyGatherer(GameObjectComponentID id)
	{
		auto iter = s_resourceDependencyGatherers.find(id);
		return iter != s_resourceDependencyGatherers.end() ? iter->second : nullptr;
	}

}