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

		// The state values of the elements of object's editable Arrays (and theirs, nested); object's properties are
		// type's. parentPath leads to the Array object is an element of, keyed elementKey, or is null for the component.
		void AddElementStateValues(const TypeDescription& type, void* object, const PropertyPath* parentPath, const std::string& elementKey,
			PathValues& outValues)
		{
			for (size_t propertyIndex = 0; propertyIndex < type.Properties.size(); ++propertyIndex)
			{
				const PropertyDescriptor& property = type.Properties[propertyIndex];
				const PropertyPath path = parentPath != nullptr ? parentPath->Nested(elementKey, propertyIndex) : PropertyPath(propertyIndex);

				if (parentPath != nullptr && IsStateValue(property, object))
				{
					outValues.emplace_back(path, property.Get(object));
				}

				if (property.Type == EPropertyType::Array && property.HasEditableElements())
				{
					const std::vector<ObjectRef> elements = std::get<std::vector<ObjectRef>>(property.GetMutable(object));
					for (size_t elementIndex = 0; elementIndex < elements.size(); ++elementIndex)
					{
						const ObjectRef& element = elements[elementIndex];
						AddElementStateValues(*element.Type, element.MutableObject, &path, GetElementKey(element, elementIndex), outValues);
					}
				}
			}
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

	ResolvedProperty Resolve(GameObjectComponentBase& component, const PropertyPath& path)
	{
		const ComponentDescription* const description = GetDescription(component.m_id);
		if (description == nullptr || path.IsEmpty())
		{
			return ResolvedProperty();
		}

		const TypeDescription* type = description->Type;
		void* object = description->ToObject(component);

		const std::vector<PropertyPath::Step>& steps = path.GetSteps();
		for (size_t stepIndex = 0; stepIndex < steps.size(); ++stepIndex)
		{
			const PropertyPath::Step& step = steps[stepIndex];
			if (step.PropertyIndex >= type->Properties.size())
			{
				return ResolvedProperty();
			}

			const PropertyDescriptor& property = type->Properties[step.PropertyIndex];
			if (stepIndex + 1 == steps.size())
			{
				ResolvedProperty resolved;
				resolved.Property = &property;
				resolved.Object = object;
				return resolved;
			}

			// Every step before the last goes into an element of an editable Array
			if (property.Type != EPropertyType::Array || !property.HasEditableElements())
			{
				return ResolvedProperty();
			}

			const std::vector<ObjectRef> elements = std::get<std::vector<ObjectRef>>(property.GetMutable(object));
			const ObjectRef* found = nullptr;
			for (size_t elementIndex = 0; elementIndex < elements.size() && found == nullptr; ++elementIndex)
			{
				if (GetElementKey(elements[elementIndex], elementIndex) == step.ElementKey)
				{
					found = &elements[elementIndex];
				}
			}
			if (found == nullptr || found->MutableObject == nullptr)
			{
				return ResolvedProperty();
			}

			type = found->Type;
			object = found->MutableObject;
		}

		return ResolvedProperty();
	}

	std::string GetElementKey(const ObjectRef& element, size_t elementIndex)
	{
		return element.Key.empty() ? std::to_string(elementIndex) : element.Key;
	}

	bool IsStateValue(const PropertyDescriptor& property, const void* object)
	{
		return property.Set && !property.IsExclusive && !property.IsEditorOnly
			&& (!property.IsOverridable() || property.IsOverridden(object));
	}

	PathValues GetElementStateValues(GameObjectComponentBase& component)
	{
		PathValues values;
		const ComponentDescription* const description = GetDescription(component.m_id);
		if (description != nullptr)
		{
			AddElementStateValues(*description->Type, description->ToObject(component), nullptr, std::string(), values);
		}
		return values;
	}

	void SetValues(GameObjectComponentBase& component, const PathValues& values)
	{
		for (const std::pair<PropertyPath, PropertyValue>& value : values)
		{
			const ResolvedProperty resolved = Resolve(component, value.first);
			if (resolved.IsValid())
			{
				resolved.Property->Set(resolved.Object, value.second);
			}
		}
	}
}
