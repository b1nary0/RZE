#include <UI/Panels/ImGuiDemoPanel.h>

#include <UI/Framework/PanelRegistry.h>

#include <ImGui/imgui.h>

namespace Editor
{
	REGISTER_EDITOR_PANEL(ImGuiDemoPanel);

	PanelDesc ImGuiDemoPanel::Describe()
	{
		PanelDesc desc;
		// ShowDemoWindow names its own window; this is only the panel's name in menus
		desc.Name = "ImGui Demo";
		desc.MenuPath = "Help/ImGui Demo";
		desc.DefaultOpen = false;
		desc.OwnsWindow = true;
		return desc;
	}

	void ImGuiDemoPanel::Draw(EditorContext& context)
	{
		ImGui::ShowDemoWindow(GetOpenFlag());
	}
}
