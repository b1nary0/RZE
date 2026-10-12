#pragma once

#include <Commands/EditorCommand.h>

#include <Game/World/GameObject/GameObjectDefinitions.h>
#include <Game/World/GameObject/Properties/PropertyDescriptor.h>

namespace Editor
{
	// Sets one described property (ComponentPropertyRegistry) on a component through its setter. Does nothing if
	// the component has been destroyed.
	class SetPropertyCommand : public EditorCommand
	{
	public:
		SetPropertyCommand(GameObjectComponentPtr<GameObjectComponentBase> component, size_t propertyIndex, PropertyValue value, InteractionID interaction);

		void Execute(EditorContext& context) override;
		const char* GetName() const override { return "Set Property"; }

	private:
		GameObjectComponentPtr<GameObjectComponentBase> m_component;
		size_t m_propertyIndex;
		PropertyValue m_value;
	};
}
