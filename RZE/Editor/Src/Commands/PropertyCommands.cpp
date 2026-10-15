#include <Commands/PropertyCommands.h>

#include <RZE.h>

#include <Game/World/GameObject/GameObject.h>
#include <Game/World/GameObject/GameObjectComponent.h>
#include <Game/World/GameObject/Properties/ComponentPropertyRegistry.h>

#include <Utils/DebugUtils/Debug.h>

#include <optional>
#include <vector>

namespace
{
	const PropertyDescriptor& GetProperty(GameObjectComponentID componentID, size_t propertyIndex)
	{
		const std::vector<PropertyDescriptor>* const properties = ComponentPropertyRegistry::GetProperties(componentID);
		AssertExpr(properties != nullptr && propertyIndex < properties->size());
		return (*properties)[propertyIndex];
	}

	// The object whose component of this type has an Exclusive property turned on, or an invalid GUID if none does
	ObjectGUID FindExclusiveHolder(GameObjectComponentID componentID, size_t propertyIndex)
	{
		const PropertyDescriptor& property = GetProperty(componentID, propertyIndex);

		ObjectGUID holder = ObjectGUIDs::k_invalid;
		RZE().GetActiveScene().ForEachGameObject(
			Functor<void, GameObjectPtr>([&](GameObjectPtr gameObject)
			{
				if (holder != ObjectGUIDs::k_invalid)
				{
					return;
				}

				std::vector<GameObjectComponentPtr<GameObjectComponentBase>> components;
				gameObject->GetComponentHandles(components);
				for (const GameObjectComponentPtr<GameObjectComponentBase>& component : components)
				{
					if (component->m_id == componentID
						&& std::get<bool>(property.Get(ComponentPropertyRegistry::GetDescribedObject(**component))))
					{
						holder = gameObject->GetGUID();
						return;
					}
				}
			}));
		return holder;
	}

	// The property's value on object as undo sees it: none for an overridable property that isn't overridden, since
	// undoing back to that state means resetting it rather than setting the base's value
	std::optional<PropertyValue> GetUndoValue(const PropertyDescriptor& property, const void* object)
	{
		if (property.IsOverridable() && !property.IsOverridden(object))
		{
			return std::nullopt;
		}
		return property.Get(object);
	}

	class SetPropertyRecord : public Editor::UndoRecord
	{
	public:
		// propertyName is the name of the property path leads to, for the Edit menu. before and after are none when the
		// (overridable) property wasn't, or isn't, overridden.
		SetPropertyRecord(ObjectGUID guid, GameObjectComponentID componentID, const PropertyPath& path, const std::string& propertyName,
			std::optional<PropertyValue>&& before, std::optional<PropertyValue>&& after)
			: m_guid(guid)
			, m_componentID(componentID)
			, m_path(path)
			, m_propertyName(propertyName)
			, m_before(std::move(before))
			, m_after(std::move(after))
		{
		}

		// For an Exclusive property: the object that had it on before, which undo turns it back on for
		void SetPreviousHolder(ObjectGUID holder) { m_previousHolder = holder; }

		// For a component's own property: its elements' state values before the change, which undo puts back after
		// the property, since changing it can replace the elements (a new mesh has new material slots) and lose them
		void SetElementValuesBefore(ComponentPropertyRegistry::PathValues&& values) { m_elementValuesBefore = std::move(values); }

		void Undo(Editor::EditorContext& context) override
		{
			// An Exclusive property is turned off by turning it on elsewhere, and its setter does that
			if (m_previousHolder != ObjectGUIDs::k_invalid)
			{
				Apply(m_previousHolder, PropertyValue(true));
			}
			else
			{
				Apply(m_guid, m_before);
			}

			if (!m_elementValuesBefore.empty())
			{
				const GameObjectComponentPtr<GameObjectComponentBase> component = Editor::UndoTargets::FindComponent(m_guid, m_componentID);
				if (component != nullptr)
				{
					ComponentPropertyRegistry::SetValues(**component, m_elementValuesBefore);
				}
			}
		}

		void Redo(Editor::EditorContext& context) override
		{
			Apply(m_guid, m_after);
		}

		std::string GetName() const override
		{
			return (m_after.has_value() ? "Change " : "Reset ") + m_propertyName;
		}

		bool TryMerge(const Editor::UndoRecord& newer) override
		{
			const SetPropertyRecord* const newerProperty = dynamic_cast<const SetPropertyRecord*>(&newer);
			if (newerProperty == nullptr || newerProperty->m_guid != m_guid
				|| newerProperty->m_componentID != m_componentID || newerProperty->m_path != m_path)
			{
				return false;
			}

			m_after = newerProperty->m_after;
			return true;
		}

	private:
		// Sets value, or resets the property for none. Does nothing if the object, component or (for a nested property)
		// element is gone.
		void Apply(ObjectGUID guid, const std::optional<PropertyValue>& value)
		{
			const GameObjectComponentPtr<GameObjectComponentBase> component = Editor::UndoTargets::FindComponent(guid, m_componentID);
			if (component == nullptr)
			{
				return;
			}

			const ComponentPropertyRegistry::ResolvedProperty resolved = ComponentPropertyRegistry::Resolve(**component, m_path);
			if (!resolved.IsValid())
			{
				return;
			}

			if (value.has_value())
			{
				resolved.Property->Set(resolved.Object, *value);
			}
			else if (resolved.Property->IsOverridable())
			{
				resolved.Property->Reset(resolved.Object);
			}
		}

	private:
		ObjectGUID m_guid;
		GameObjectComponentID m_componentID;
		PropertyPath m_path;
		std::string m_propertyName;
		std::optional<PropertyValue> m_before;
		std::optional<PropertyValue> m_after;
		ObjectGUID m_previousHolder = ObjectGUIDs::k_invalid;
		ComponentPropertyRegistry::PathValues m_elementValuesBefore;
	};
}

namespace Editor
{
	SetPropertyCommand::SetPropertyCommand(const GameObjectComponentPtr<GameObjectComponentBase>& component, const PropertyPath& path, PropertyValue&& value, InteractionID interaction)
		: EditorCommand(interaction)
		, m_component(component)
		, m_path(path)
		, m_value(std::move(value))
	{
	}

	bool SetPropertyCommand::Execute(EditorContext& context)
	{
		if (m_component == nullptr || !UndoTargets::IsInScene(m_component->GetOwner()))
		{
			return false;
		}

		const ComponentPropertyRegistry::ResolvedProperty resolved = ComponentPropertyRegistry::Resolve(**m_component, m_path);
		if (!resolved.IsValid())
		{
			return false;
		}

		const PropertyDescriptor& property = *resolved.Property;
		AssertMsg(property.IsEditable(), "Read-only properties can't be set");
		AssertMsg(!property.IsExclusive || !m_path.IsNested(), "Only a component's own properties can be Exclusive");

		// A view setting rather than part of the scene: applied, but not undoable and doesn't count as a change
		if (property.IsEditorOnly)
		{
			property.Set(resolved.Object, m_value);
			return false;
		}

		const GameObjectComponentID componentID = m_component->m_id;
		std::unique_ptr<SetPropertyRecord> record = std::make_unique<SetPropertyRecord>(
			m_component->GetOwner()->GetGUID(), componentID, m_path, property.Name, GetUndoValue(property, resolved.Object), std::optional<PropertyValue>(m_value));
		if (property.IsExclusive)
		{
			record->SetPreviousHolder(FindExclusiveHolder(componentID, m_path.GetSteps().front().PropertyIndex));
		}
		if (!m_path.IsNested())
		{
			record->SetElementValuesBefore(ComponentPropertyRegistry::GetElementStateValues(**m_component));
		}

		property.Set(resolved.Object, m_value);
		SetUndoRecord(std::move(record));
		return true;
	}

	ResetPropertyCommand::ResetPropertyCommand(const GameObjectComponentPtr<GameObjectComponentBase>& component, const PropertyPath& path, InteractionID interaction)
		: EditorCommand(interaction)
		, m_component(component)
		, m_path(path)
	{
	}

	bool ResetPropertyCommand::Execute(EditorContext& context)
	{
		if (m_component == nullptr || !UndoTargets::IsInScene(m_component->GetOwner()))
		{
			return false;
		}

		const ComponentPropertyRegistry::ResolvedProperty resolved = ComponentPropertyRegistry::Resolve(**m_component, m_path);
		if (!resolved.IsValid())
		{
			return false;
		}

		const PropertyDescriptor& property = *resolved.Property;
		if (!property.IsOverridable() || !property.IsOverridden(resolved.Object))
		{
			return false;
		}
		AssertMsg(property.IsEditable(), "Read-only properties can't be reset");

		std::unique_ptr<SetPropertyRecord> record = std::make_unique<SetPropertyRecord>(
			m_component->GetOwner()->GetGUID(), m_component->m_id, m_path, property.Name, GetUndoValue(property, resolved.Object), std::nullopt);

		property.Reset(resolved.Object);
		// A view setting isn't part of the scene, so isn't undoable
		if (property.IsEditorOnly)
		{
			return false;
		}

		SetUndoRecord(std::move(record));
		return true;
	}
}
