#pragma once

#include <Game/World/GameObject/GameObjectDefinitions.h>
#include <Game/World/GameObject/Properties/PropertyBuilder.h>
#include <Game/World/GameObject/Properties/PropertyPath.h>

#include <type_traits>
#include <utility>
#include <vector>

// Every registered component type's description, keyed by component type. Filled when a component type is
// registered (REGISTER_GAMEOBJECTCOMPONENT), from its static DescribeProperties if it has one. Startup and main
// thread only for writes.
namespace ComponentPropertyRegistry
{
	template <typename T, typename = void>
	struct HasDescribeProperties : std::false_type {};

	template <typename T>
	struct HasDescribeProperties<T, std::void_t<decltype(T::DescribeProperties(std::declval<PropertyBuilder<T>&>()))>>
		: std::true_type {};

	struct ComponentDescription
	{
		const TypeDescription* Type = nullptr;
		// The component as the type its description describes, which property accessors expect
		void* (*ToObject)(GameObjectComponentBase& component) = nullptr;
	};

	void SetDescription(GameObjectComponentID id, const ComponentDescription& description);

	// Null for a component type that doesn't describe its properties
	const ComponentDescription* GetDescription(GameObjectComponentID id);
	const std::vector<PropertyDescriptor>* GetProperties(GameObjectComponentID id);

	// The component as its description's object, for PropertyDescriptor::Get/Set; null if its type isn't described
	void* GetDescribedObject(GameObjectComponentBase& component);

	// Where a PropertyPath leads on a component: the property, and the object to Get or Set it on
	struct ResolvedProperty
	{
		// Null when the path leads nowhere: its element is gone, or a step isn't an editable Array
		const PropertyDescriptor* Property = nullptr;
		void* Object = nullptr;

		bool IsValid() const { return Property != nullptr; }
	};
	ResolvedProperty Resolve(GameObjectComponentBase& component, const PropertyPath& path);

	// How a PropertyPath refers to an Array element: its Key, or its index when it has none
	std::string GetElementKey(const ObjectRef& element, size_t elementIndex);

	// Whether property's value on object is part of the object's state, which copies take and undo puts back: it has
	// a setter, isn't Exclusive (a copy mustn't take over from the original, e.g. as the main camera) or EditorOnly (a
	// view setting), and, if it's overridable, is overridden (otherwise it comes from the base)
	bool IsStateValue(const PropertyDescriptor& property, const void* object);

	using PathValues = std::vector<std::pair<PropertyPath, PropertyValue>>;
	// The state values (IsStateValue) of the elements of the component's editable Arrays, nested ones included, each
	// with its path. Not the component's own properties.
	PathValues GetElementStateValues(GameObjectComponentBase& component);
	// Sets each value whose path still leads somewhere on the component; ones that don't (e.g. an element it hasn't
	// got) are skipped
	void SetValues(GameObjectComponentBase& component, const PathValues& values);

	template <typename TComponent>
	void Register()
	{
		if constexpr (HasDescribeProperties<TComponent>::value)
		{
			ComponentDescription description;
			description.Type = &DescribeType<TComponent>();
			description.ToObject = [](GameObjectComponentBase& component) -> void*
				{
					return static_cast<TComponent*>(&component);
				};
			SetDescription(TComponent::GetID(), description);
		}
	}
}
