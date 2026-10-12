#pragma once

#include <Game/World/GameObject/GameObjectDefinitions.h>
#include <Game/World/GameObject/Properties/PropertyBuilder.h>

#include <type_traits>
#include <utility>
#include <vector>

// Every registered component type's PropertyDescriptors, keyed by component type. Filled when a component type is
// registered (REGISTER_GAMEOBJECTCOMPONENT), from its static DescribeProperties if it has one. Startup and main
// thread only for writes.
namespace ComponentPropertyRegistry
{
	template <typename TComponent, typename = void>
	struct HasDescribeProperties : std::false_type {};

	template <typename TComponent>
	struct HasDescribeProperties<TComponent, std::void_t<decltype(TComponent::DescribeProperties(std::declval<PropertyBuilder<TComponent>&>()))>>
		: std::true_type {};

	void SetProperties(GameObjectComponentID id, std::vector<PropertyDescriptor>&& properties);

	// Null for a component type that doesn't describe its properties
	const std::vector<PropertyDescriptor>* GetProperties(GameObjectComponentID id);

	template <typename TComponent>
	void Register()
	{
		if constexpr (HasDescribeProperties<TComponent>::value)
		{
			std::vector<PropertyDescriptor> properties;
			PropertyBuilder<TComponent> builder(properties);
			TComponent::DescribeProperties(builder);
			SetProperties(TComponent::GetID(), std::move(properties));
		}
	}
}
