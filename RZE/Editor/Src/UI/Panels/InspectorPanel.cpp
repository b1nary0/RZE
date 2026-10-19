#include <UI/Panels/InspectorPanel.h>

#include <UI/Framework/EditorContext.h>
#include <UI/Framework/EditorUI.h>
#include <UI/Framework/PanelRegistry.h>

#include <Game/World/GameObject/EditorGameComponentInfoCache.h>
#include <Game/World/GameObject/GameObject.h>

#include <ImGui/imgui.h>

#include <algorithm>

namespace
{
	U32 GetInspectorOrder(const GameObjectComponentBase& component)
	{
		const EditorComponentCache::ComponentInfoMap& componentInfo = EditorComponentCache::GetAllComponentsInfo();
		const auto it = componentInfo.find(component.m_id);
		return it != componentInfo.end() ? it->second.Order : EditorComponentCache::kDefaultSortingOrder;
	}
}

namespace Editor
{
	REGISTER_EDITOR_PANEL(InspectorPanel);

	PanelDesc InspectorPanel::Describe()
	{
		PanelDesc desc;
		desc.Name = "Object Inspector";
		desc.MenuPath = "View/Windows/Object Inspector";
		desc.DefaultDock = EPanelDock::Right;
		return desc;
	}

	void InspectorPanel::Update(EditorContext& context)
	{
		m_propertyInspector.UpdatePendingEdits(context);

		m_componentGroups.clear();

		const EditorSelection& selection = context.GetSelection();
		m_selectedCount = selection.GetAll().size();

		const GameObjectPtr primary = selection.GetPrimary();
		if (primary == nullptr)
		{
			return;
		}

		// Stable, so components with the same order keep the order they were added in
		PropertyInspector::ComponentList primaryComponents;
		primary->GetComponentHandles(primaryComponents);
		std::stable_sort(primaryComponents.begin(), primaryComponents.end(),
			[](const GameObjectComponentPtr<GameObjectComponentBase>& a, const GameObjectComponentPtr<GameObjectComponentBase>& b)
			{
				return GetInspectorOrder(**a) < GetInspectorOrder(**b);
			});

		// Every other selected object's components, to match by type
		std::vector<PropertyInspector::ComponentList> otherComponents;
		for (const GameObjectPtr& gameObject : selection.GetAll())
		{
			if (gameObject != primary && gameObject != nullptr)
			{
				otherComponents.emplace_back();
				gameObject->GetComponentHandles(otherComponents.back());
			}
		}

		for (const GameObjectComponentPtr<GameObjectComponentBase>& component : primaryComponents)
		{
			PropertyInspector::ComponentList group = { component };
			for (const PropertyInspector::ComponentList& components : otherComponents)
			{
				const auto match = std::find_if(components.begin(), components.end(),
					[&component](const GameObjectComponentPtr<GameObjectComponentBase>& other)
					{
						return other->m_id == component->m_id;
					});
				if (match == components.end())
				{
					break;
				}
				group.push_back(*match);
			}

			// Only types every selected object has
			if (group.size() == otherComponents.size() + 1)
			{
				m_componentGroups.push_back(std::move(group));
			}
		}
	}

	void InspectorPanel::OnSceneReset()
	{
		// They were for the scene that's going
		m_propertyInspector.CancelPendingEdits();
	}

	void InspectorPanel::Draw(EditorContext& context)
	{
		m_propertyInspector.BeginFrame();

		if (m_selectedCount > 1)
		{
			ImGui::TextDisabled("%d objects selected; showing the components they share", static_cast<int>(m_selectedCount));
		}

		for (const PropertyInspector::ComponentList& group : m_componentGroups)
		{
			const GameObjectComponentPtr<GameObjectComponentBase>& component = group.front();
			if (component == nullptr)
			{
				continue;
			}

			const std::string heading = "[ " + component->GetName() + " ]";
			EditorUI::SectionHeading(heading.c_str());
			m_propertyInspector.Draw(context, group);
			ImGui::Separator();
		}

		m_propertyInspector.EndFrame();
	}
}
