#include <Game/World/GameObject/GameObjectComponent.h>

#include <Utils/LiveIDAllocator.h>

namespace
{
	// Hands out every GameObjectComponentInstanceID (see GameObjectComponentLifetime). Made on first use and never
	// destroyed, so it works for components made during static initialization or destroyed after main returns.
	LiveIDAllocator& AliveComponents()
	{
		static LiveIDAllocator* const s_allocator = new LiveIDAllocator();
		return *s_allocator;
	}
}

bool GameObjectComponentLifetime::IsAlive(GameObjectComponentInstanceID id)
{
	return AliveComponents().Contains(id);
}

GameObjectComponentInstanceID GameObjectComponentLifetime::GetID(const GameObjectComponentBase* component)
{
	return component->GetInstanceID();
}

GameObjectComponentBase::GameObjectComponentBase(GameObjectComponentID typeID)
	: m_typeID(typeID)
	, m_instanceID(AliveComponents().Acquire())
{
}

GameObjectComponentBase::~GameObjectComponentBase()
{
	AliveComponents().Release(m_instanceID);
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