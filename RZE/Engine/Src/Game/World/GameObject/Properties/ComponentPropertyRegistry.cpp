#include <StdAfx.h>
#include <Game/World/GameObject/Properties/ComponentPropertyRegistry.h>

#include <Game/World/GameObject/GameObjectComponent.h>

#include <unordered_map>

namespace ComponentPropertyRegistry
{
	namespace
	{
		// A function-local static, so it exists before any static-time registration uses it
		std::unordered_map<GameObjectComponentID, ComponentDescription>& GetRegistry()
		{
			static std::unordered_map<GameObjectComponentID, ComponentDescription> s_registry;
			return s_registry;
		}
	}

	void SetDescription(GameObjectComponentID id, const ComponentDescription& description)
	{
		GetRegistry()[id] = description;
	}

	const ComponentDescription* GetDescription(GameObjectComponentID id)
	{
		const auto it = GetRegistry().find(id);
		return it != GetRegistry().end() ? &it->second : nullptr;
	}

	const std::vector<PropertyDescriptor>* GetProperties(GameObjectComponentID id)
	{
		const ComponentDescription* const description = GetDescription(id);
		return description != nullptr ? &description->Type->Properties : nullptr;
	}

	void* GetDescribedObject(GameObjectComponentBase& component)
	{
		const ComponentDescription* const description = GetDescription(component.m_id);
		return description != nullptr ? description->ToObject(component) : nullptr;
	}
}
