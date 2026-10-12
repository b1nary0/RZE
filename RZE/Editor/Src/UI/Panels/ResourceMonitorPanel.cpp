#include <UI/Panels/ResourceMonitorPanel.h>

#include <UI/Framework/PanelRegistry.h>

#include <RZE.h>
#include <EngineCore/Resources/ResourceHandler.h>

#include <ImGui/imgui.h>

#include <algorithm>

namespace
{
	constexpr double k_refreshIntervalMS = 250.0;
}

namespace Editor
{
	REGISTER_EDITOR_PANEL(ResourceMonitorPanel);

	PanelDesc ResourceMonitorPanel::Describe()
	{
		PanelDesc desc;
		desc.Name = "Resource Monitor";
		desc.MenuPath = "View/Windows/Resource Monitor";
		desc.DefaultDock = EPanelDock::Left;
		desc.DefaultOpen = false;
		return desc;
	}

	void ResourceMonitorPanel::PushWindowStyle(PanelWindowStyle& style) const
	{
		style.Push(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));
		style.Push(ImGuiStyleVar_FramePadding, ImVec2(12.f, ImGui::GetStyle().FramePadding.y));
	}

	void ResourceMonitorPanel::Update(EditorContext& context)
	{
		if (m_hasRows && m_sinceRefresh.GetElapsedMS<double>() < k_refreshIntervalMS)
		{
			return;
		}
		m_hasRows = true;
		m_sinceRefresh.Start();

		// The table is an unordered_map, so copy it out sorted for a stable display
		const auto& resourceTable = RZE().GetResourceHandler().GetResourceTableInternal();
		m_rows.resize(resourceTable.size());

		size_t rowIndex = 0;
		for (const auto& resource : resourceTable)
		{
			ResourceRow& row = m_rows[rowIndex++];
			row.m_name = resource.first;
			row.m_refCount = resource.second.GetRefCount();
		}

		std::sort(m_rows.begin(), m_rows.end(), [](const ResourceRow& a, const ResourceRow& b)
			{
				return a.m_name < b.m_name;
			});
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

			// Only the visible rows are drawn
			ImGuiListClipper clipper;
			clipper.Begin(static_cast<int>(m_rows.size()));
			while (clipper.Step())
			{
				for (int rowIndex = clipper.DisplayStart; rowIndex < clipper.DisplayEnd; ++rowIndex)
				{
					const ResourceRow& row = m_rows[rowIndex];
					ImGui::TableNextRow();

					ImGui::TableSetColumnIndex(0);
					ImGui::TextUnformatted(row.m_name.c_str());

					ImGui::TableSetColumnIndex(1);
					ImGui::Text("%d", row.m_refCount);
				}
			}

			ImGui::EndTable();
		}
	}
}
