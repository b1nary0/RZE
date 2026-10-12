#include <StdAfx.h>
#include <Game/World/GameObject/Properties/ComponentPropertyRegistry.h>

#include <unordered_map>

namespace ComponentPropertyRegistry
{
	namespace
	{
		// A function-local static, so it exists before any static-time registration uses it
		std::unordered_map<GameObjectComponentID, std::vector<PropertyDescriptor>>& GetRegistry()
		{
			static std::unordered_map<GameObjectComponentID, std::vector<PropertyDescriptor>> s_registry;
			return s_registry;
		}
	}

	void SetProperties(GameObjectComponentID id, std::vector<PropertyDescriptor>&& properties)
	{
		GetRegistry()[id] = std::move(properties);
	}

	const std::vector<PropertyDescriptor>* GetProperties(GameObjectComponentID id)
	{
		const auto it = GetRegistry().find(id);
		return it != GetRegistry().end() ? &it->second : nullptr;
	}
}
