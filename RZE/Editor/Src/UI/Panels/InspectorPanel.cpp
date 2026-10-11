#include <UI/Panels/InspectorPanel.h>

#include <UI/Framework/EditorContext.h>
#include <UI/Framework/PanelRegistry.h>

#include <Game/World/GameObject/EditorGameComponentInfoCache.h>
#include <Game/World/GameObject/GameObject.h>

#include <ImGui/imgui.h>

#include <algorithm>

namespace
{
	U32 GetInspectorOrder(const GameObjectComponentBase* component)
	{
		const EditorComponentCache::ComponentInfoMap& componentInfo = EditorComponentCache::GetAllComponentsInfo();
		const auto it = componentInfo.find(component->m_id);
		return it != componentInfo.end() ? it->second.Order : EditorComponentCache::kDefaultSortingOrder;
	}
}

namespace Editor
{
	REGISTER_EDITOR_PANEL(InspectorPanel);

	PanelDesc InspectorPanel::Describe()
	{
		PanelDesc desc;
		desc.Name = "Component View";
		desc.MenuPath = "Windows/Component View";
		return desc;
	}

	void InspectorPanel::Draw(EditorContext& context)
	{
		const GameObjectPtr primary = context.GetSelection().GetPrimary();
		if (primary == nullptr)
		{
			return;
		}

		// Stable, so components with the same order keep the order they were added in
		const GameObject::ComponentList& components = primary->GetComponents();
		m_sortedComponents.assign(components.begin(), components.end());
		std::stable_sort(m_sortedComponents.begin(), m_sortedComponents.end(), [](const GameObjectComponentBase* a, const GameObjectComponentBase* b)
			{
				return GetInspectorOrder(a) < GetInspectorOrder(b);
			});

		for (GameObjectComponentBase* component : m_sortedComponents)
		{
			ImGui::TextColored(ImVec4(0.65f, 0.65f, 1.0f, 1.0f), "[ %s ]", component->GetName().c_str());
			ImGui::Separator();
			component->OnEditorInspect();
			ImGui::Separator();
		}
	}
}
