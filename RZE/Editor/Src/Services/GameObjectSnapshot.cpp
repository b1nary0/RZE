#include <Services/GameObjectSnapshot.h>

#include <RZE.h>

#include <Game/World/GameObject/GameObject.h>
#include <Game/World/GameObject/GameObjectComponent.h>
#include <Game/World/GameObject/Properties/ComponentPropertyRegistry.h>

namespace
{
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
					if (ComponentPropertyRegistry::IsStateValue(property, object))
					{
						componentValues.Values.emplace_back(PropertyPath(index), property.Get(object));
					}
				}

				// After the component's own, so they're set once the elements exist
				const ComponentPropertyRegistry::PathValues elementValues = ComponentPropertyRegistry::GetElementStateValues(*component);
				componentValues.Values.insert(componentValues.Values.end(), elementValues.begin(), elementValues.end());
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

			// In order, so the elements exist by the time their values are set; skipped if the copy hasn't got one
			ComponentPropertyRegistry::SetValues(*component, componentValues.Values);
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
