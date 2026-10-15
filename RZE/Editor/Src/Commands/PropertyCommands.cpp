#include <Commands/PropertyCommands.h>

#include <RZE.h>

#include <Game/World/GameObject/GameObject.h>
#include <Game/World/GameObject/GameObjectComponent.h>
#include <Game/World/GameObject/Properties/ComponentPropertyRegistry.h>

#include <Utils/DebugUtils/Debug.h>

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

	class SetPropertyRecord : public Editor::UndoRecord
	{
	public:
		// propertyName is the name of the property path leads to, for the Edit menu
		SetPropertyRecord(ObjectGUID guid, GameObjectComponentID componentID, const PropertyPath& path, const std::string& propertyName,
			PropertyValue&& before, PropertyValue&& after)
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
		}

		void Redo(Editor::EditorContext& context) override
		{
			Apply(m_guid, m_after);
		}

		std::string GetName() const override
		{
			return "Change " + m_propertyName;
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
		// Does nothing if the object, component or (for a nested property) element is gone
		void Apply(ObjectGUID guid, const PropertyValue& value)
		{
			const GameObjectComponentPtr<GameObjectComponentBase> component = Editor::UndoTargets::FindComponent(guid, m_componentID);
			if (component == nullptr)
			{
				return;
			}

			const ComponentPropertyRegistry::ResolvedProperty resolved = ComponentPropertyRegistry::Resolve(**component, m_path);
			if (resolved.IsValid())
			{
				resolved.Property->Set(resolved.Object, value);
			}
		}

	private:
		ObjectGUID m_guid;
		GameObjectComponentID m_componentID;
		PropertyPath m_path;
		std::string m_propertyName;
		PropertyValue m_before;
		PropertyValue m_after;
		ObjectGUID m_previousHolder = ObjectGUIDs::k_invalid;
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
			m_component->GetOwner()->GetGUID(), componentID, m_path, property.Name, property.Get(resolved.Object), PropertyValue(m_value));
		if (property.IsExclusive)
		{
			record->SetPreviousHolder(FindExclusiveHolder(componentID, m_path.GetSteps().front().PropertyIndex));
		}

		property.Set(resolved.Object, m_value);
		SetUndoRecord(std::move(record));
		return true;
	}
}
