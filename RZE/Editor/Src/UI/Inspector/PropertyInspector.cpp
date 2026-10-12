#include <UI/Inspector/PropertyInspector.h>

#include <UI/Framework/EditorContext.h>
#include <UI/Inspector/PropertyDrawers.h>

#include <Commands/PropertyCommands.h>

#include <Game/World/GameObject/GameObjectComponent.h>
#include <Game/World/GameObject/Properties/ComponentPropertyRegistry.h>

#include <ImGui/imgui.h>

namespace
{
	constexpr float k_labelColumnFraction = 0.35f;
}

namespace Editor
{
	void PropertyInspector::BeginFrame()
	{
		m_isHeldThisFrame = false;
	}

	void PropertyInspector::EndFrame()
	{
		if (!m_isHeldThisFrame)
		{
			m_heldEdit = HeldEdit();
		}
	}

	bool PropertyInspector::Draw(EditorContext& context, const GameObjectComponentPtr<GameObjectComponentBase>& component)
	{
		const std::vector<PropertyDescriptor>* const properties = ComponentPropertyRegistry::GetProperties(component->m_id);
		if (properties == nullptr || properties->empty())
		{
			return false;
		}

		const GameObjectComponentInstanceID instanceID = component->GetInstanceID();
		ImGui::PushID(static_cast<int>(instanceID));

		if (ImGui::BeginTable("##properties", 2, ImGuiTableFlags_SizingStretchProp))
		{
			ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch, k_labelColumnFraction);
			ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch, 1.0f - k_labelColumnFraction);

			for (size_t propertyIndex = 0; propertyIndex < properties->size(); ++propertyIndex)
			{
				const PropertyDescriptor& property = (*properties)[propertyIndex];
				const PropertyDrawers::Drawer drawer = PropertyDrawers::Find(property.Type);
				if (drawer == nullptr)
				{
					continue;
				}

				ImGui::PushID(static_cast<int>(propertyIndex));
				ImGui::TableNextRow();

				ImGui::TableSetColumnIndex(0);
				ImGui::AlignTextToFramePadding();
				ImGui::TextUnformatted(property.Name.c_str());
				if (!property.Tooltip.empty() && ImGui::IsItemHovered())
				{
					ImGui::SetTooltip("%s", property.Tooltip.c_str());
				}

				ImGui::TableSetColumnIndex(1);
				ImGui::SetNextItemWidth(-FLT_MIN);

				// Edits a copy; the component only changes when the command runs
				PropertyValue value = property.Get(**component);

				ImGui::BeginDisabled(property.IsReadOnly());
				const bool changed = drawer(property, value);
				const bool isHeld = ImGui::IsItemActive();
				ImGui::EndDisabled();

				if (isHeld && m_heldEdit.m_component == instanceID && m_heldEdit.m_propertyIndex == propertyIndex)
				{
					m_isHeldThisFrame = true;
				}

				if (changed && !property.IsReadOnly())
				{
					const InteractionID interaction = GetInteraction(instanceID, propertyIndex, isHeld);
					context.GetCommands().Push<SetPropertyCommand>(component, propertyIndex, std::move(value), interaction);
				}

				ImGui::PopID();
			}

			ImGui::EndTable();
		}

		ImGui::PopID();
		return true;
	}

	InteractionID PropertyInspector::GetInteraction(GameObjectComponentInstanceID component, size_t propertyIndex, bool isHeld)
	{
		if (!isHeld)
		{
			return NewInteractionID();
		}

		const bool isSameEdit = m_heldEdit.m_interaction != k_noInteraction
			&& m_heldEdit.m_component == component && m_heldEdit.m_propertyIndex == propertyIndex;
		if (!isSameEdit)
		{
			m_heldEdit.m_component = component;
			m_heldEdit.m_propertyIndex = propertyIndex;
			m_heldEdit.m_interaction = NewInteractionID();
		}

		m_isHeldThisFrame = true;
		return m_heldEdit.m_interaction;
	}
}
