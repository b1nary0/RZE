#pragma once

#include <Commands/EditorCommand.h>

#include <Game/World/GameObject/GameObjectDefinitions.h>
#include <Game/World/GameObject/Properties/PropertyDescriptor.h>
#include <Game/World/GameObject/Properties/PropertyPath.h>

namespace Editor
{
	// Sets one of a component's described properties (ComponentPropertyRegistry), or a property of an element of
	// one of its editable Arrays, through its setter. Does nothing if the component has been destroyed or the path
	// no longer leads anywhere (e.g. the element is gone).
	class SetPropertyCommand : public EditorCommand
	{
	public:
		SetPropertyCommand(const GameObjectComponentPtr<GameObjectComponentBase>& component, const PropertyPath& path, PropertyValue&& value, InteractionID interaction);

		bool Execute(EditorContext& context) override;
		const char* GetName() const override { return "Set Property"; }

	private:
		GameObjectComponentPtr<GameObjectComponentBase> m_component;
		PropertyPath m_path;
		PropertyValue m_value;
	};

	// Resets an overridable property (PropertyDescriptor::IsOverridable) back to its base, undoably. Does nothing if
	// it isn't overridden, or SetPropertyCommand's conditions aren't met.
	class ResetPropertyCommand : public EditorCommand
	{
	public:
		ResetPropertyCommand(const GameObjectComponentPtr<GameObjectComponentBase>& component, const PropertyPath& path, InteractionID interaction);

		bool Execute(EditorContext& context) override;
		const char* GetName() const override { return "Reset Property"; }

	private:
		GameObjectComponentPtr<GameObjectComponentBase> m_component;
		PropertyPath m_path;
	};
}
