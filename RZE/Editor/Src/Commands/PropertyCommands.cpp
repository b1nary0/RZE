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
		SetPropertyRecord(ObjectGUID guid, GameObjectComponentID componentID, size_t propertyIndex, PropertyValue before, PropertyValue after)
			: m_guid(guid)
			, m_componentID(componentID)
			, m_propertyIndex(propertyIndex)
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
			return "Change " + GetProperty(m_componentID, m_propertyIndex).Name;
		}

		bool TryMerge(const Editor::UndoRecord& newer) override
		{
			const SetPropertyRecord* const newerProperty = dynamic_cast<const SetPropertyRecord*>(&newer);
			if (newerProperty == nullptr || newerProperty->m_guid != m_guid
				|| newerProperty->m_componentID != m_componentID || newerProperty->m_propertyIndex != m_propertyIndex)
			{
				return false;
			}

			m_after = newerProperty->m_after;
			return true;
		}

	private:
		void Apply(ObjectGUID guid, const PropertyValue& value)
		{
			const GameObjectComponentPtr<GameObjectComponentBase> component = Editor::UndoTargets::FindComponent(guid, m_componentID);
			if (component != nullptr)
			{
				GetProperty(m_componentID, m_propertyIndex).Set(ComponentPropertyRegistry::GetDescribedObject(**component), value);
			}
		}

	private:
		ObjectGUID m_guid;
		GameObjectComponentID m_componentID;
		size_t m_propertyIndex;
		PropertyValue m_before;
		PropertyValue m_after;
		ObjectGUID m_previousHolder = ObjectGUIDs::k_invalid;
	};
}

namespace Editor
{
	SetPropertyCommand::SetPropertyCommand(GameObjectComponentPtr<GameObjectComponentBase> component, size_t propertyIndex, PropertyValue value, InteractionID interaction)
		: EditorCommand(interaction)
		, m_component(component)
		, m_propertyIndex(propertyIndex)
		, m_value(std::move(value))
	{
	}

	bool SetPropertyCommand::Execute(EditorContext& context)
	{
		if (m_component == nullptr)
		{
			return false;
		}

		const GameObjectComponentID componentID = m_component->m_id;
		const PropertyDescriptor& property = GetProperty(componentID, m_propertyIndex);
		AssertMsg(!property.IsReadOnly(), "Read-only properties can't be set");

		void* const object = ComponentPropertyRegistry::GetDescribedObject(**m_component);

		// A view setting rather than part of the scene: applied, but not undoable and doesn't count as a change
		if (property.IsEditorOnly)
		{
			property.Set(object, m_value);
			return false;
		}

		std::unique_ptr<SetPropertyRecord> record = std::make_unique<SetPropertyRecord>(
			m_component->GetOwner()->GetGUID(), componentID, m_propertyIndex, property.Get(object), m_value);
		if (property.IsExclusive)
		{
			record->SetPreviousHolder(FindExclusiveHolder(componentID, m_propertyIndex));
		}

		property.Set(object, m_value);
		SetUndoRecord(std::move(record));
		return true;
	}
}
