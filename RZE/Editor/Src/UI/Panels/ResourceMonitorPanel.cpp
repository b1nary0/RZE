#include <UI/Panels/ResourceMonitorPanel.h>

#include <UI/Framework/PanelRegistry.h>

#include <RZE.h>
#include <EngineCore/Resources/ResourceHandler.h>

#include <ImGui/imgui.h>

namespace Editor
{
	REGISTER_EDITOR_PANEL(ResourceMonitorPanel);

	PanelDesc ResourceMonitorPanel::Describe()
	{
		PanelDesc desc;
		desc.Name = "Resource Monitor";
		desc.MenuPath = "Windows/Resource Monitor";
		desc.DefaultOpen = false;
		return desc;
	}

	void ResourceMonitorPanel::PushWindowStyle(PanelWindowStyle& style) const
	{
		style.Push(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));
		style.Push(ImGuiStyleVar_FramePadding, ImVec2(12.f, ImGui::GetStyle().FramePadding.y));
	}

	void ResourceMonitorPanel::Draw(EditorContext& context)
	{
		ImVec2 size = ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y);
		static ImGuiTableFlags flags =
			ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable
			| ImGuiTableFlags_RowBg | ImGuiTableFlags_NoBordersInBody
			| ImGuiTableFlags_ScrollY
			| ImGuiTableFlags_SizingStretchProp;

		if (ImGui::BeginTable("##ResourceTable", 2, flags, size))
		{
			ImGui::TableSetupColumn("Resource Name", ImGuiTableColumnFlags_WidthStretch, -1.f);
			ImGui::TableSetupColumn("References", ImGuiTableColumnFlags_DefaultSort | ImGuiTableColumnFlags_WidthFixed, 15.f);
			ImGui::TableSetupScrollFreeze(1, 1);
			ImGui::TableHeadersRow();

			const auto& resourceList = RZE().GetResourceHandler().GetResourceTableInternal();

			// TODO (mitch) CLIPPING: Implement clipping to only draw the resources only being displayed
			// this has some implications on how ImGui does sorting.

			// TODO (mitch) SORTING: Unfortunately sorting can't be applied here atm with a unordered_map type
			// need to have a trivially sortable container
			// For now we just render the unordered list of resources and their ref count.
			for (auto& resource : resourceList)
			{
				ImGui::TableNextRow();

				ImGui::TableSetColumnIndex(0);
				ImGui::Text(resource.first.c_str());

				ImGui::TableSetColumnIndex(1);
				ImGui::Text(std::to_string(resource.second.GetRefCount()).c_str());
			}

			ImGui::EndTable();
		}
	}
}
