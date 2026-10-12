#include <UI/Panels/InspectorPanel.h>

#include <UI/Framework/EditorContext.h>
#include <UI/Framework/EditorUI.h>
#include <UI/Framework/PanelRegistry.h>
#include <UI/Inspector/ComponentInspectors.h>

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
		const GameObjectPtr primary = context.GetSelection().GetPrimary();
		if (primary == nullptr)
		{
			m_sortedComponents.clear();
			return;
		}

		// Stable, so components with the same order keep the order they were added in
		primary->GetComponentHandles(m_sortedComponents);
		std::stable_sort(m_sortedComponents.begin(), m_sortedComponents.end(),
			[](const GameObjectComponentPtr<GameObjectComponentBase>& a, const GameObjectComponentPtr<GameObjectComponentBase>& b)
			{
				return GetInspectorOrder(**a) < GetInspectorOrder(**b);
			});
	}

	void InspectorPanel::Draw(EditorContext& context)
	{
		m_propertyInspector.BeginFrame();

		for (const GameObjectComponentPtr<GameObjectComponentBase>& component : m_sortedComponents)
		{
			if (component == nullptr)
			{
				continue;
			}

			const std::string heading = "[ " + component->GetName() + " ]";
			EditorUI::SectionHeading(heading.c_str());
			m_propertyInspector.Draw(context, component);

			if (const ComponentInspectors::Inspector* const customInspector = ComponentInspectors::Find(component->m_id))
			{
				ImGui::PushID(static_cast<int>(component->GetInstanceID()));
				(*customInspector)(context, **component);
				ImGui::PopID();
			}
			ImGui::Separator();
		}

		m_propertyInspector.EndFrame();
	}
}
