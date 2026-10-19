#include <UI/Inspector/PropertyInspector.h>

#include <UI/Framework/EditorContext.h>
#include <UI/Framework/EditorUI.h>
#include <UI/Inspector/PropertyDrawers.h>

#include <Commands/PropertyCommands.h>

#include <Game/World/GameObject/GameObjectComponent.h>
#include <Game/World/GameObject/Properties/ComponentPropertyRegistry.h>

#include <DebugUtils/DebugServices.h>

#include <EngineCore/Resources/PendingResource.h>

#include <Utils/StringUtils.h>

#include <ImGui/imgui.h>
#include <ImGui/imgui_internal.h> // ImGuiItemFlags_MixedValue

#include <algorithm>
#include <cctype>
#include <cstring>
#include <string>

namespace
{
	// Beside a field whose asset is loading
	constexpr char k_loadingText[] = "Loading...";

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
		case EPropertyType::Enum:
			return true;
		default:
			return false;
		}
	}

	std::string GetDisplayName(const ObjectRef& ref)
	{
		return ref.IsValid() && ref.Type->GetDisplayName ? ref.Type->GetDisplayName(ref.Object) : std::string();
	}

	// How many of the type's values object overrides
	size_t CountOverridden(const TypeDescription& type, const void* object)
	{
		size_t count = 0;
		for (const PropertyDescriptor& property : type.Properties)
		{
			if (property.IsOverridable() && property.IsOverridden(object))
			{
				++count;
			}
		}
		return count;
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

	PropertyPath PropertyInspector::PathPrefix::PathTo(size_t propertyIndex) const
	{
		return Parent != nullptr ? Parent->Nested(ElementKey, propertyIndex) : PropertyPath(propertyIndex);
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
			DrawProperties(context, *description->Type, object, PathPrefix(), &components);
			EditorUI::EndPropertyTable();
		}
		ImGui::PopID();
		return true;
	}

	void PropertyInspector::DrawProperties(EditorContext& context, const TypeDescription& type, const void* object, const PathPrefix& prefix,
		const ComponentList* editableComponents)
	{
		// The group the rows drawn so far are in, so a sub-heading only starts a group
		std::string currentGroup;

		for (size_t propertyIndex = 0; propertyIndex < type.Properties.size(); ++propertyIndex)
		{
			const PropertyDescriptor& property = type.Properties[propertyIndex];
			const PropertyPath path = prefix.PathTo(propertyIndex);

			ImGui::PushID(static_cast<int>(propertyIndex));

			// Each group starts with its section row, and ungrouped rows after one are set apart by a bare line. A
			// preview is a section of its own, named after it.
			const std::string& group = property.Type == EPropertyType::Preview ? property.Name : property.Group;
			if (group != currentGroup)
			{
				currentGroup = group;
				EditorUI::PropertySectionRow(currentGroup.c_str());
			}

			if (property.Type == EPropertyType::Object)
			{
				ImGui::TableNextRow();
				const ObjectRef ref = std::get<ObjectRef>(property.Get(object));
				const std::string displayName = GetDisplayName(ref);
				const std::string heading = displayName.empty() ? property.Name : property.Name + ": " + displayName;
				// Nested objects are read-only; only an editable Array's elements can be edited
				DrawObjectNode(context, heading.c_str(), ref, PathPrefix(), nullptr);
			}
			else if (property.Type == EPropertyType::Array)
			{
				const std::vector<ObjectRef> elements = std::get<std::vector<ObjectRef>>(property.Get(object));
				// The elements can be edited if the Array allows it and these properties can be
				const ComponentList* const elementComponents = property.HasEditableElements() ? editableComponents : nullptr;

				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				const bool isOpen = ImGui::TreeNodeEx("##array", ImGuiTreeNodeFlags_SpanAllColumns | ImGuiTreeNodeFlags_AllowOverlap,
					"%s (%d)", property.Name.c_str(), static_cast<int>(elements.size()));

				// A long list gets a filter, beside its heading
				const std::string* filter = nullptr;
				if (isOpen && elements.size() > k_filterThreshold)
				{
					std::string& filterText = m_listFilters[ImGui::GetID("##filter")];
					ImGui::TableSetColumnIndex(1);
					ImGui::SetNextItemWidth(-FLT_MIN);
					char buffer[128];
					strncpy_s(buffer, filterText.c_str(), _TRUNCATE);
					if (ImGui::InputTextWithHint("##filter", "Filter", buffer, sizeof(buffer)))
					{
						filterText = buffer;
					}
					filter = &filterText;
				}

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

						if (filter != nullptr && !StringUtils::ContainsIgnoringCase(heading, *filter))
						{
							continue;
						}

						PathPrefix elementPrefix;
						elementPrefix.Parent = &path;
						elementPrefix.ElementKey = ComponentPropertyRegistry::GetElementKey(element, elementIndex);

						ImGui::PushID(static_cast<int>(elementIndex));
						ImGui::TableNextRow();
						DrawObjectNode(context, heading.c_str(), element, elementPrefix, elementComponents);
						ImGui::PopID();
					}
					ImGui::TreePop();
				}
			}
			else if (property.Type == EPropertyType::Preview)
			{
				DrawPreviewRow(property, path, editableComponents);
			}
			else
			{
				DrawValueRow(context, property, path, object, editableComponents);
			}

			ImGui::PopID();
		}
	}

	void PropertyInspector::DrawValueRow(EditorContext& context, const PropertyDescriptor& property, const PropertyPath& path, const void* object,
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

		// Whether the other selected components' values differ from the first's, which is the one shown. One whose
		// path leads nowhere (e.g. without the same element) isn't counted.
		bool isMixed = false;
		if (editableComponents != nullptr)
		{
			for (size_t targetIndex = 1; targetIndex < editableComponents->size() && !isMixed; ++targetIndex)
			{
				const GameObjectComponentPtr<GameObjectComponentBase>& target = (*editableComponents)[targetIndex];
				if (target == nullptr)
				{
					continue;
				}

				const ComponentPropertyRegistry::ResolvedProperty resolved = ComponentPropertyRegistry::Resolve(**target, path);
				isMixed = resolved.IsValid() && !AreEqual(before, resolved.Property->Get(resolved.Object));
			}
		}

		const bool isOverridden = property.IsOverridable() && property.IsOverridden(object);
		if (isMixed)
		{
			EditorUI::PropertyRow(property.Name.c_str(), "Differs across the selection; showing the primary's value", k_mixedValueColour);
		}
		else if (isOverridden)
		{
			EditorUI::PropertyRow(property.Name.c_str(), property.Tooltip.empty() ? "Overridden" : property.Tooltip.c_str(), EditorUI::k_accentColour);
		}
		else
		{
			EditorUI::PropertyRow(property.Name.c_str(), property.Tooltip.c_str());
		}

		// Why it can't be changed at the moment, shown on hover. An exclusive property is turned off by turning it on
		// for another object, and can't be on for several at once.
		const char* readOnlyReason = nullptr;
		if (property.IsExclusive && editableComponents != nullptr)
		{
			if (editableComponents->size() > 1)
			{
				readOnlyReason = "Only one object can have this on; select just one to change it";
			}
			else if (std::get<bool>(before))
			{
				readOnlyReason = "Turn this on for another one to change it";
			}
		}
		if (readOnlyReason == nullptr && property.IsEnabled && !property.IsEnabled(object))
		{
			readOnlyReason = property.DisabledReason.c_str();
		}

		// Read-only nested objects (editableComponents null) can't be edited: commands only reach a component's own
		// properties and its editable Arrays' elements
		const bool isReadOnly = !property.IsEditable() || editableComponents == nullptr || readOnlyReason != nullptr;

		// An overridden value can be reset even while it can't be changed, from a button beside it
		const bool canReset = isOverridden && editableComponents != nullptr && property.IsEditable();
		const bool isLoading = editableComponents != nullptr && editableComponents->front() != nullptr
			&& IsEditPending(editableComponents->front()->GetInstanceID(), path);
		const float spacing = ImGui::GetStyle().ItemInnerSpacing.x;
		float besideWidth = canReset ? EditorUI::GetIconButtonWidth() + spacing : 0.0f;
		besideWidth += isLoading ? ImGui::CalcTextSize(k_loadingText).x + spacing : 0.0f;
		if (besideWidth > 0.0f)
		{
			ImGui::SetNextItemWidth(-besideWidth);
		}

		const bool isDisabled = isReadOnly && IsDrawnAsWidget(property.Type);
		ImGui::BeginDisabled(isDisabled);
		// Checkboxes show a dash for mixed values
		ImGui::PushItemFlag(ImGuiItemFlags_MixedValue, isMixed);
		const bool changed = drawer(context, property, object, value, isReadOnly);
		ImGui::PopItemFlag();
		const bool isHeld = ImGui::IsItemActive();
		if (readOnlyReason != nullptr && readOnlyReason[0] != '\0' && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
		{
			ImGui::SetTooltip("%s", readOnlyReason);
		}
		ImGui::EndDisabled();

		if (isLoading)
		{
			ImGui::SameLine(0.0f, spacing);
			ImGui::AlignTextToFramePadding();
			ImGui::TextDisabled("%s", k_loadingText);
			EditorUI::ItemTooltip("It's set once it has loaded");
		}

		if (canReset)
		{
			ImGui::SameLine(0.0f, spacing);
			if (EditorUI::IconButton("##reset", EditorUI::EIcon::Reset, "Reset"))
			{
				const InteractionID interaction = NewInteractionID();
				for (const GameObjectComponentPtr<GameObjectComponentBase>& target : *editableComponents)
				{
					if (target != nullptr)
					{
						context.GetCommands().Push<ResetPropertyCommand>(target, path, interaction);
					}
				}
			}
		}

		if (editableComponents == nullptr || isReadOnly)
		{
			return;
		}

		// The held-edit bookkeeping follows the first component, which every target's edit shares
		const GameObjectComponentInstanceID instanceID = editableComponents->front()->GetInstanceID();
		if (isHeld && m_heldEdit.m_component == instanceID && m_heldEdit.m_path == path)
		{
			m_isHeldThisFrame = true;
		}

		if (!changed)
		{
			return;
		}

		// An asset that has to load first is set once it's in, so a large one doesn't stall the editor
		if (DeferEditUntilLoaded(property, path, before, value, *editableComponents))
		{
			return;
		}

		PushEdit(context, *editableComponents, path, before, value, GetInteraction(instanceID, path, isHeld));
	}

	void PropertyInspector::DrawPreviewRow(const PropertyDescriptor& property, const PropertyPath& path, const ComponentList* editableComponents)
	{
		ComponentPropertyRegistry::ResolvedProperty resolved;
		if (editableComponents != nullptr && editableComponents->front() != nullptr)
		{
			resolved = ComponentPropertyRegistry::Resolve(**editableComponents->front(), path);
		}

		const float width = EditorUI::BeginPropertyWideRow();
		if (resolved.IsValid())
		{
			const PropertyTexture preview = property.MakePreview(resolved.Object);
			// At its own size, which it's made for, unless that's wider than the row
			const float scale = preview.Size.X() > width ? width / preview.Size.X() : 1.0f;
			PropertyDrawers::DrawTexture(preview, std::max(preview.Size.X(), preview.Size.Y()) * scale);
		}
		else
		{
			ImGui::TextDisabled("(no preview)");
		}
		EditorUI::EndPropertyWideRow();
	}

	void PropertyInspector::DrawObjectNode(EditorContext& context, const char* heading, const ObjectRef& ref, const PathPrefix& prefix,
		const ComponentList* editableComponents)
	{
		ImGui::TableSetColumnIndex(0);
		if (!ref.IsValid())
		{
			ImGui::TextDisabled("%s (none)", heading);
			return;
		}

		const TypeDescription& type = *ref.Type;
		const size_t overriddenCount = CountOverridden(type, ref.Object);

		// The arrow, then the icon (if the type has one), then the heading, drawn by hand so the icon sits between
		const bool isOpen = ImGui::TreeNodeEx("##object", ImGuiTreeNodeFlags_SpanAllColumns | ImGuiTreeNodeFlags_AllowOverlap, "%s", "");
		if (type.GetIcon)
		{
			ImGui::SameLine();
			PropertyDrawers::DrawThumbnail(type.GetIcon(ref.Object), ImGui::GetTextLineHeight());
		}
		ImGui::SameLine();
		if (overriddenCount == 0)
		{
			// Nothing sits beside it in the value column, so a long heading can run across the row rather than being
			// cut off at the label column
			const ImVec2 start = ImGui::GetCursorScreenPos();
			const float rowRight = ImGui::GetWindowPos().x + ImGui::GetWindowWidth() - ImGui::GetStyle().WindowPadding.x;
			ImGui::PushClipRect(start, ImVec2(rowRight, start.y + ImGui::GetTextLineHeightWithSpacing()), false);
			ImGui::TextUnformatted(heading);
			ImGui::PopClipRect();
		}
		else
		{
			ImGui::TextUnformatted(heading);
		}

		// How many of its values are overridden, and a button that resets them all
		if (overriddenCount > 0)
		{
			ImGui::TableSetColumnIndex(1);
			ImGui::TextColored(EditorUI::k_accentColour, "%d overridden", static_cast<int>(overriddenCount));
			if (editableComponents != nullptr)
			{
				ImGui::SameLine();
				if (EditorUI::IconButton("##resetAll", EditorUI::EIcon::Reset, "Reset all"))
				{
					ResetOverridden(context, type, prefix, *editableComponents);
				}
			}
		}

		if (isOpen)
		{
			DrawProperties(context, type, ref.Object, prefix, editableComponents);
			ImGui::TreePop();
		}
	}

	void PropertyInspector::ResetOverridden(EditorContext& context, const TypeDescription& type, const PathPrefix& prefix,
		const ComponentList& editableComponents)
	{
		const InteractionID interaction = NewInteractionID();
		for (size_t propertyIndex = 0; propertyIndex < type.Properties.size(); ++propertyIndex)
		{
			if (!type.Properties[propertyIndex].IsOverridable())
			{
				continue;
			}

			const PropertyPath path = prefix.PathTo(propertyIndex);
			for (const GameObjectComponentPtr<GameObjectComponentBase>& target : editableComponents)
			{
				if (target != nullptr)
				{
					context.GetCommands().Push<ResetPropertyCommand>(target, path, interaction);
				}
			}
		}
	}

	struct PropertyInspector::PendingEdits
	{
		struct Edit
		{
			ComponentList Targets;
			// The first target's, which identifies the field with Path
			GameObjectComponentInstanceID Component = GameObjectLifetime::k_invalidID;
			PropertyPath Path;
			PropertyValue Before;
			PropertyValue Value;
			InteractionID Interaction = k_noInteraction;
			// The asset Value names
			PendingResource Asset;
		};

		std::vector<Edit> Edits;
		// Assets whose edits were pushed this UI frame, held until their commands have run
		std::vector<PendingResource> HandedOver;
	};

	PropertyInspector::PropertyInspector() = default;

	PropertyInspector::~PropertyInspector() = default;

	void PropertyInspector::UpdatePendingEdits(EditorContext& context)
	{
		if (m_pendingEdits == nullptr)
		{
			return;
		}

		// Their commands ran at the end of the last UI frame
		m_pendingEdits->HandedOver.clear();

		std::vector<PendingEdits::Edit>& edits = m_pendingEdits->Edits;
		for (auto edit = edits.begin(); edit != edits.end();)
		{
			if (edit->Asset.IsLoading())
			{
				++edit;
				continue;
			}

			if (edit->Asset.HasFailed())
			{
				DebugServices::Get().Trace(LogChannel::Info, "Couldn't load " + edit->Asset.GetPath().GetRelativePath() + ", so it wasn't set.");
			}
			else
			{
				// Loaded, so the setters take it from the cache rather than loading it on the main thread
				PushEdit(context, edit->Targets, edit->Path, edit->Before, edit->Value, edit->Interaction);
				m_pendingEdits->HandedOver.push_back(std::move(edit->Asset));
			}
			edit = edits.erase(edit);
		}

		if (edits.empty() && m_pendingEdits->HandedOver.empty())
		{
			m_pendingEdits = nullptr;
		}
	}

	void PropertyInspector::CancelPendingEdits()
	{
		m_pendingEdits = nullptr;
	}

	void PropertyInspector::PushEdit(EditorContext& context, const ComponentList& targets, const PropertyPath& path, const PropertyValue& before,
		const PropertyValue& value, InteractionID interaction)
	{
		for (size_t targetIndex = 0; targetIndex < targets.size(); ++targetIndex)
		{
			const GameObjectComponentPtr<GameObjectComponentBase>& target = targets[targetIndex];
			if (target == nullptr)
			{
				continue;
			}

			if (targetIndex == 0)
			{
				context.GetCommands().Push<SetPropertyCommand>(target, path, PropertyValue(value), interaction);
				continue;
			}

			const ComponentPropertyRegistry::ResolvedProperty resolved = ComponentPropertyRegistry::Resolve(**target, path);
			if (resolved.IsValid())
			{
				context.GetCommands().Push<SetPropertyCommand>(target, path, ApplyEdit(before, value, resolved.Property->Get(resolved.Object)), interaction);
			}
		}
	}

	bool PropertyInspector::DeferEditUntilLoaded(const PropertyDescriptor& property, const PropertyPath& path, const PropertyValue& before,
		const PropertyValue& value, const ComponentList& targets)
	{
		if (!property.LoadAsset || targets.empty() || targets.front() == nullptr)
		{
			return false;
		}

		// A newer edit to the field replaces one still loading, even one that's made now
		const GameObjectComponentInstanceID component = targets.front()->GetInstanceID();
		if (m_pendingEdits != nullptr)
		{
			std::vector<PendingEdits::Edit>& edits = m_pendingEdits->Edits;
			edits.erase(std::remove_if(edits.begin(), edits.end(),
				[component, &path](const PendingEdits::Edit& edit) { return edit.Component == component && edit.Path == path; }), edits.end());
		}

		// Clearing it loads nothing
		const Filepath& asset = std::get<Filepath>(value);
		if (!asset.IsValid())
		{
			return false;
		}

		PendingEdits::Edit edit;
		if (property.LoadAsset(edit.Asset, asset, EAsyncOperationFlags::Background) == nullptr)
		{
			// Already loaded
			return false;
		}

		edit.Targets = targets;
		edit.Component = component;
		edit.Path = path;
		edit.Before = before;
		edit.Value = value;
		edit.Interaction = NewInteractionID();
		if (m_pendingEdits == nullptr)
		{
			m_pendingEdits = std::make_unique<PendingEdits>();
		}
		m_pendingEdits->Edits.push_back(std::move(edit));
		return true;
	}

	bool PropertyInspector::IsEditPending(GameObjectComponentInstanceID component, const PropertyPath& path) const
	{
		if (m_pendingEdits == nullptr)
		{
			return false;
		}
		return std::any_of(m_pendingEdits->Edits.begin(), m_pendingEdits->Edits.end(),
			[component, &path](const PendingEdits::Edit& edit) { return edit.Component == component && edit.Path == path; });
	}

	InteractionID PropertyInspector::GetInteraction(GameObjectComponentInstanceID component, const PropertyPath& path, bool isHeld)
	{
		if (!isHeld)
		{
			return NewInteractionID();
		}

		const bool isSameEdit = m_heldEdit.m_interaction != k_noInteraction
			&& m_heldEdit.m_component == component && m_heldEdit.m_path == path;
		if (!isSameEdit)
		{
			m_heldEdit.m_component = component;
			m_heldEdit.m_path = path;
			m_heldEdit.m_interaction = NewInteractionID();
		}

		m_isHeldThisFrame = true;
		return m_heldEdit.m_interaction;
	}
}
