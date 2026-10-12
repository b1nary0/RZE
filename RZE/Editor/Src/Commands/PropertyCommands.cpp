#include <Commands/PropertyCommands.h>

#include <Game/World/GameObject/GameObjectComponent.h>
#include <Game/World/GameObject/Properties/ComponentPropertyRegistry.h>

#include <Utils/DebugUtils/Debug.h>

namespace Editor
{
	SetPropertyCommand::SetPropertyCommand(GameObjectComponentPtr<GameObjectComponentBase> component, size_t propertyIndex, PropertyValue value, InteractionID interaction)
		: EditorCommand(interaction)
		, m_component(component)
		, m_propertyIndex(propertyIndex)
		, m_value(std::move(value))
	{
	}

	void SetPropertyCommand::Execute(EditorContext& context)
	{
		if (m_component == nullptr)
		{
			return;
		}

		const std::vector<PropertyDescriptor>* const properties = ComponentPropertyRegistry::GetProperties(m_component->m_id);
		AssertExpr(properties != nullptr && m_propertyIndex < properties->size());

		const PropertyDescriptor& property = (*properties)[m_propertyIndex];
		AssertMsg(!property.IsReadOnly(), "Read-only properties can't be set");

		property.Set(ComponentPropertyRegistry::GetDescribedObject(**m_component), m_value);
	}
}
