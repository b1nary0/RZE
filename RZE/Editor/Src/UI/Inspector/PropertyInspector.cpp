#include <UI/Inspector/PropertyInspector.h>

#include <UI/Framework/EditorContext.h>
#include <UI/Framework/EditorUI.h>
#include <UI/Inspector/PropertyDrawers.h>

#include <Commands/PropertyCommands.h>

#include <Game/World/GameObject/GameObjectComponent.h>
#include <Game/World/GameObject/Properties/ComponentPropertyRegistry.h>

#include <ImGui/imgui.h>

#include <algorithm>
#include <string>

namespace
{
	// Read-only values of these types are drawn as disabled widgets; the rest (text, paths, images) draw read-only
	// themselves
	bool IsDrawnAsWidget(EPropertyType type)
	{
		switch (type)
		{
		case EPropertyType::Bool:
		case EPropertyType::Int:
		case EPropertyType::Float:
		case EPropertyType::Vector3:
		case EPropertyType::Vector4:
			return true;
		default:
			return false;
		}
	}

	std::string GetDisplayName(const ObjectRef& ref)
	{
		return ref.IsValid() && ref.Type->GetDisplayName ? ref.Type->GetDisplayName(ref.Object) : std::string();
	}
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
		const ComponentPropertyRegistry::ComponentDescription* const description = ComponentPropertyRegistry::GetDescription(component->m_id);
		if (description == nullptr || description->Type->Properties.empty())
		{
			return false;
		}

		const void* const object = description->ToObject(**component);

		// Wide enough for this component's own labels; nested ones are indented and show in full on hover if cut off
		float widestLabel = 0.0f;
		for (const PropertyDescriptor& property : description->Type->Properties)
		{
			widestLabel = std::max(widestLabel, ImGui::CalcTextSize(property.Name.c_str()).x);
		}

		ImGui::PushID(static_cast<int>(component->GetInstanceID()));
		if (EditorUI::BeginPropertyTable("##properties", EditorUI::CalcLabelColumnWidth(widestLabel)))
		{
			DrawProperties(context, *description->Type, object, &component);
			EditorUI::EndPropertyTable();
		}
		ImGui::PopID();
		return true;
	}

	void PropertyInspector::DrawProperties(EditorContext& context, const TypeDescription& type, const void* object,
		const GameObjectComponentPtr<GameObjectComponentBase>* editableComponent)
	{
		for (size_t propertyIndex = 0; propertyIndex < type.Properties.size(); ++propertyIndex)
		{
			const PropertyDescriptor& property = type.Properties[propertyIndex];

			ImGui::PushID(static_cast<int>(propertyIndex));

			if (property.Type == EPropertyType::Object)
			{
				ImGui::TableNextRow();
				const ObjectRef ref = std::get<ObjectRef>(property.Get(object));
				const std::string displayName = GetDisplayName(ref);
				const std::string heading = displayName.empty() ? property.Name : property.Name + ": " + displayName;
				DrawObjectNode(context, heading.c_str(), ref);
			}
			else if (property.Type == EPropertyType::Array)
			{
				const std::vector<ObjectRef> elements = std::get<std::vector<ObjectRef>>(property.Get(object));

				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				const bool isOpen = ImGui::TreeNodeEx("##array", ImGuiTreeNodeFlags_SpanAllColumns, "%s (%d)", property.Name.c_str(), static_cast<int>(elements.size()));
				if (isOpen)
				{
					for (size_t elementIndex = 0; elementIndex < elements.size(); ++elementIndex)
					{
						const ObjectRef& element = elements[elementIndex];
						std::string heading = !element.Label.empty() ? element.Label : GetDisplayName(element);
						if (heading.empty())
						{
							heading = "[" + std::to_string(elementIndex) + "]";
						}

						ImGui::PushID(static_cast<int>(elementIndex));
						ImGui::TableNextRow();
						DrawObjectNode(context, heading.c_str(), element);
						ImGui::PopID();
					}
					ImGui::TreePop();
				}
			}
			else
			{
				DrawValueRow(context, property, propertyIndex, object, editableComponent);
			}

			ImGui::PopID();
		}
	}

	void PropertyInspector::DrawValueRow(EditorContext& context, const PropertyDescriptor& property, size_t propertyIndex, const void* object,
		const GameObjectComponentPtr<GameObjectComponentBase>* editableComponent)
	{
		const PropertyDrawers::Drawer drawer = PropertyDrawers::Find(property.Type);
		if (drawer == nullptr)
		{
			return;
		}

		EditorUI::PropertyRow(property.Name.c_str(), property.Tooltip.c_str());

		// Nested objects' properties can't be edited yet: commands only address a component's own properties
		const bool isReadOnly = property.IsReadOnly() || editableComponent == nullptr;

		// Edits a copy; the component only changes when the command runs
		PropertyValue value = property.Get(object);

		const bool isDisabled = isReadOnly && IsDrawnAsWidget(property.Type);
		ImGui::BeginDisabled(isDisabled);
		const bool changed = drawer(property, value, isReadOnly);
		const bool isHeld = ImGui::IsItemActive();
		ImGui::EndDisabled();

		if (editableComponent == nullptr || isReadOnly)
		{
			return;
		}

		const GameObjectComponentPtr<GameObjectComponentBase>& component = *editableComponent;
		const GameObjectComponentInstanceID instanceID = component->GetInstanceID();
		if (isHeld && m_heldEdit.m_component == instanceID && m_heldEdit.m_propertyIndex == propertyIndex)
		{
			m_isHeldThisFrame = true;
		}

		if (changed)
		{
			const InteractionID interaction = GetInteraction(instanceID, propertyIndex, isHeld);
			context.GetCommands().Push<SetPropertyCommand>(component, propertyIndex, std::move(value), interaction);
		}
	}

	void PropertyInspector::DrawObjectNode(EditorContext& context, const char* heading, const ObjectRef& ref)
	{
		ImGui::TableSetColumnIndex(0);
		if (!ref.IsValid())
		{
			ImGui::TextDisabled("%s (none)", heading);
			return;
		}

		if (ImGui::TreeNodeEx("##object", ImGuiTreeNodeFlags_SpanAllColumns, "%s", heading))
		{
			DrawProperties(context, *ref.Type, ref.Object, nullptr);
			ImGui::TreePop();
		}
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
