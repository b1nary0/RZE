#include <Services/GameObjectSnapshot.h>

#include <RZE.h>

#include <Game/World/GameObject/GameObject.h>
#include <Game/World/GameObject/GameObjectComponent.h>
#include <Game/World/GameObject/Properties/ComponentPropertyRegistry.h>

namespace
{
	// An overridable value is only copied when it's overridden: otherwise it comes from the copy's own base
	bool IsCopied(const PropertyDescriptor& property, const void* object)
	{
		return property.Set && !property.IsExclusive && !property.IsEditorOnly
			&& (!property.IsOverridable() || property.IsOverridden(object));
	}

	// The copied properties of each element of object's editable Arrays (and theirs, nested), whose properties are
	// type's, with their paths from the component; parentPath is the path to object's Array, or empty for the
	// component itself
	void TakeElementValues(const TypeDescription& type, void* object, const PropertyPath* parentPath, const std::string& elementKey,
		std::vector<std::pair<PropertyPath, PropertyValue>>& outValues)
	{
		for (size_t propertyIndex = 0; propertyIndex < type.Properties.size(); ++propertyIndex)
		{
			const PropertyDescriptor& property = type.Properties[propertyIndex];
			const PropertyPath path = parentPath != nullptr ? parentPath->Nested(elementKey, propertyIndex) : PropertyPath(propertyIndex);

			if (parentPath != nullptr && IsCopied(property, object))
			{
				outValues.emplace_back(path, property.Get(object));
			}

			if (property.Type == EPropertyType::Array && property.HasEditableElements())
			{
				const std::vector<ObjectRef> elements = std::get<std::vector<ObjectRef>>(property.GetMutable(object));
				for (size_t elementIndex = 0; elementIndex < elements.size(); ++elementIndex)
				{
					const ObjectRef& element = elements[elementIndex];
					TakeElementValues(*element.Type, element.MutableObject, &path, ComponentPropertyRegistry::GetElementKey(element, elementIndex), outValues);
				}
			}
		}
	}

	// The object's component of this type, adding one if it hasn't got one (every object starts with a Transform)
	GameObjectComponentBase* FindOrAddComponent(GameObject& gameObject, GameObjectComponentID id)
	{
		for (GameObjectComponentBase* component : gameObject.GetComponents())
		{
			if (component->m_id == id)
			{
				return component;
			}
		}
		return gameObject.AddComponentByID(id);
	}
}

namespace Editor
{
	GameObjectSnapshot GameObjectSnapshot::Take(GameObject& gameObject)
	{
		GameObjectSnapshot snapshot;
		snapshot.m_name = gameObject.GetName();
		snapshot.m_includeInSave = gameObject.IncludeInSave();
		if (gameObject.GetParent() != nullptr)
		{
			snapshot.m_parentGUID = gameObject.GetParent()->GetGUID();
		}

		for (GameObjectComponentBase* component : gameObject.GetComponents())
		{
			ComponentValues componentValues;
			componentValues.ID = component->m_id;

			// A component type without a description has nothing to copy, but the copy still gets one
			const ComponentPropertyRegistry::ComponentDescription* const description = ComponentPropertyRegistry::GetDescription(component->m_id);
			if (description != nullptr)
			{
				void* const object = description->ToObject(*component);
				const std::vector<PropertyDescriptor>& properties = description->Type->Properties;
				for (size_t index = 0; index < properties.size(); ++index)
				{
					const PropertyDescriptor& property = properties[index];
					if (IsCopied(property, object))
					{
						componentValues.Values.emplace_back(PropertyPath(index), property.Get(object));
					}
				}

				// After the component's own, so they're set once the elements exist
				TakeElementValues(*description->Type, object, nullptr, std::string(), componentValues.Values);
			}

			snapshot.m_components.push_back(std::move(componentValues));
		}

		for (int childIndex = 0; childIndex < gameObject.NumChildren(); ++childIndex)
		{
			snapshot.m_children.push_back(Take(**gameObject.GetChildAtIndex(childIndex)));
		}

		return snapshot;
	}

	GameObjectPtr GameObjectSnapshot::Create(GameScene& scene, const GameObjectPtr& parent, const NameMaker& makeName) const
	{
		GameObjectPtr gameObject = scene.AddGameObject(makeName(m_name));

		for (const ComponentValues& componentValues : m_components)
		{
			GameObjectComponentBase* const component = FindOrAddComponent(**gameObject, componentValues.ID);
			if (component == nullptr || componentValues.Values.empty())
			{
				continue;
			}

			for (const std::pair<PropertyPath, PropertyValue>& value : componentValues.Values)
			{
				// Skipped if the copy hasn't got the element
				const ComponentPropertyRegistry::ResolvedProperty resolved = ComponentPropertyRegistry::Resolve(*component, value.first);
				if (resolved.IsValid())
				{
					resolved.Property->Set(resolved.Object, value.second);
				}
			}
		}

		// After the components, so the copy matches the original even if one of them sets it
		gameObject->SetIncludeInSave(m_includeInSave);

		// Positions are relative to the parent, so they don't change on attaching
		if (parent != nullptr)
		{
			gameObject->AttachTo(parent);
		}

		for (const GameObjectSnapshot& child : m_children)
		{
			child.Create(scene, gameObject, makeName);
		}

		return gameObject;
	}
}
