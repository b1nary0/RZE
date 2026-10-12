#include <UI/Inspector/PropertyInspector.h>

#include <UI/Framework/EditorContext.h>
#include <UI/Framework/EditorUI.h>
#include <UI/Inspector/PropertyDrawers.h>

#include <Commands/PropertyCommands.h>

#include <Game/World/GameObject/GameObjectComponent.h>
#include <Game/World/GameObject/Properties/ComponentPropertyRegistry.h>

#include <ImGui/imgui.h>
#include <ImGui/imgui_internal.h> // ImGuiItemFlags_MixedValue

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

	// A property's label when the selected components' values differ
	const ImVec4 k_mixedValueColour(1.0f, 0.75f, 0.3f, 1.0f);

	bool AreEqual(const Vector3D& a, const Vector3D& b) { return a == b; }
	bool AreEqual(const Vector4D& a, const Vector4D& b)
	{
		return a.X() == b.X() && a.Y() == b.Y() && a.Z() == b.Z() && a.W() == b.W();
	}
	bool AreEqual(const Filepath& a, const Filepath& b) { return a.GetRelativePath() == b.GetRelativePath(); }
	bool AreEqual(bool a, bool b) { return a == b; }
	bool AreEqual(int a, int b) { return a == b; }
	bool AreEqual(float a, float b) { return a == b; }
	bool AreEqual(const std::string& a, const std::string& b) { return a == b; }
	// Nested objects and images are never edited across a selection, so they don't count as mixed
	template <typename T>
	bool AreEqual(const T&, const T&) { return true; }

	bool AreEqual(const PropertyValue& a, const PropertyValue& b)
	{
		if (a.index() != b.index())
		{
			return false;
		}
		return std::visit([&b](const auto& aValue)
			{
				using T = std::decay_t<decltype(aValue)>;
				return AreEqual(aValue, std::get<T>(b));
			}, a);
	}

	// One target's value after an edit made on the first: a vector only takes the axes that were edited, so typing
	// a new X into a mixed selection leaves each object's Y and Z alone. Anything else takes the edit as is.
	PropertyValue ApplyEdit(const PropertyValue& before, const PropertyValue& after, const PropertyValue& target)
	{
		auto mergeAxes = [](const float* before, const float* after, float* target, int count)
			{
				for (int axis = 0; axis < count; ++axis)
				{
					if (before[axis] != after[axis])
					{
						target[axis] = after[axis];
					}
				}
			};

		if (std::holds_alternative<Vector3D>(after) && std::holds_alternative<Vector3D>(target))
		{
			Vector3D before3 = std::get<Vector3D>(before);
			Vector3D after3 = std::get<Vector3D>(after);
			Vector3D merged = std::get<Vector3D>(target);
			mergeAxes(before3.GetValuePtr(), after3.GetValuePtr(), merged.GetValuePtr(), 3);
			return PropertyValue(merged);
		}

		if (std::holds_alternative<Vector4D>(after) && std::holds_alternative<Vector4D>(target))
		{
			Vector4D before4 = std::get<Vector4D>(before);
			Vector4D after4 = std::get<Vector4D>(after);
			Vector4D merged = std::get<Vector4D>(target);
			mergeAxes(before4.GetValuePtr(), after4.GetValuePtr(), merged.GetValuePtr(), 4);
			return PropertyValue(merged);
		}

		return after;
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

	bool PropertyInspector::Draw(EditorContext& context, const ComponentList& components)
	{
		if (components.empty() || components.front() == nullptr)
		{
			return false;
		}

		const GameObjectComponentPtr<GameObjectComponentBase>& component = components.front();
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
			DrawProperties(context, *description->Type, object, &components);
			EditorUI::EndPropertyTable();
		}
		ImGui::PopID();
		return true;
	}

	void PropertyInspector::DrawProperties(EditorContext& context, const TypeDescription& type, const void* object,
		const ComponentList* editableComponents)
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
				DrawValueRow(context, property, propertyIndex, object, editableComponents);
			}

			ImGui::PopID();
		}
	}

	void PropertyInspector::DrawValueRow(EditorContext& context, const PropertyDescriptor& property, size_t propertyIndex, const void* object,
		const ComponentList* editableComponents)
	{
		const PropertyDrawers::Drawer drawer = PropertyDrawers::Find(property.Type);
		if (drawer == nullptr)
		{
			return;
		}

		// Edits a copy; the components only change when the commands run
		const PropertyValue before = property.Get(object);
		PropertyValue value = before;

		// Whether the other selected components' values differ from the first's, which is the one shown
		bool isMixed = false;
		if (editableComponents != nullptr)
		{
			for (size_t targetIndex = 1; targetIndex < editableComponents->size() && !isMixed; ++targetIndex)
			{
				const GameObjectComponentPtr<GameObjectComponentBase>& target = (*editableComponents)[targetIndex];
				isMixed = target != nullptr && !AreEqual(before, property.Get(ComponentPropertyRegistry::GetDescribedObject(**target)));
			}
		}

		if (isMixed)
		{
			ImGui::PushStyleColor(ImGuiCol_Text, k_mixedValueColour);
			EditorUI::PropertyRow(property.Name.c_str(), "Differs across the selection; showing the primary's value");
			ImGui::PopStyleColor();
		}
		else
		{
			EditorUI::PropertyRow(property.Name.c_str(), property.Tooltip.c_str());
		}

		// Nested objects' properties can't be edited yet: commands only address a component's own properties
		const bool isReadOnly = property.IsReadOnly() || editableComponents == nullptr;

		const bool isDisabled = isReadOnly && IsDrawnAsWidget(property.Type);
		ImGui::BeginDisabled(isDisabled);
		// Checkboxes show a dash for mixed values
		ImGui::PushItemFlag(ImGuiItemFlags_MixedValue, isMixed);
		const bool changed = drawer(property, value, isReadOnly);
		ImGui::PopItemFlag();
		const bool isHeld = ImGui::IsItemActive();
		ImGui::EndDisabled();

		if (editableComponents == nullptr || isReadOnly)
		{
			return;
		}

		// The held-edit bookkeeping follows the first component, which every target's edit shares
		const GameObjectComponentInstanceID instanceID = editableComponents->front()->GetInstanceID();
		if (isHeld && m_heldEdit.m_component == instanceID && m_heldEdit.m_propertyIndex == propertyIndex)
		{
			m_isHeldThisFrame = true;
		}

		if (!changed)
		{
			return;
		}

		const InteractionID interaction = GetInteraction(instanceID, propertyIndex, isHeld);
		for (size_t targetIndex = 0; targetIndex < editableComponents->size(); ++targetIndex)
		{
			const GameObjectComponentPtr<GameObjectComponentBase>& target = (*editableComponents)[targetIndex];
			if (target == nullptr)
			{
				continue;
			}

			PropertyValue targetValue = targetIndex == 0
				? value
				: ApplyEdit(before, value, property.Get(ComponentPropertyRegistry::GetDescribedObject(**target)));
			context.GetCommands().Push<SetPropertyCommand>(target, propertyIndex, std::move(targetValue), interaction);
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
