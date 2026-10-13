#include <Services/GameObjectSnapshot.h>

#include <RZE.h>

#include <Game/World/GameObject/GameObject.h>
#include <Game/World/GameObject/GameObjectComponent.h>
#include <Game/World/GameObject/Properties/ComponentPropertyRegistry.h>

namespace
{
	bool IsCopied(const PropertyDescriptor& property)
	{
		return property.Set && !property.IsExclusive && !property.IsEditorOnly;
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
			const std::vector<PropertyDescriptor>* const properties = ComponentPropertyRegistry::GetProperties(component->m_id);
			if (properties != nullptr)
			{
				const void* const object = ComponentPropertyRegistry::GetDescribedObject(*component);
				for (size_t index = 0; index < properties->size(); ++index)
				{
					const PropertyDescriptor& property = (*properties)[index];
					if (IsCopied(property))
					{
						componentValues.Values.emplace_back(index, property.Get(object));
					}
				}
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

			const std::vector<PropertyDescriptor>& properties = *ComponentPropertyRegistry::GetProperties(componentValues.ID);
			void* const object = ComponentPropertyRegistry::GetDescribedObject(*component);
			for (const std::pair<size_t, PropertyValue>& value : componentValues.Values)
			{
				properties[value.first].Set(object, value.second);
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
