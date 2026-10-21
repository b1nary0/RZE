#include <UI/Panels/SettingsPanel.h>

#include <UI/Framework/PanelRegistry.h>
#include <UI/Settings/SettingsPageRegistry.h>

#include <ImGui/imgui.h>

namespace
{
	// Font sizes across, so it follows the font
	constexpr float k_pageListWidth = 10.0f;
	// Pixels, like the file browser's title
	constexpr float k_headingFontSize = 15.0f;
}

namespace Editor
{
	REGISTER_EDITOR_PANEL(SettingsPanel);

	PanelDesc SettingsPanel::Describe()
	{
		PanelDesc desc;
		desc.Name = "Settings";
		desc.MenuPath = "Edit/Settings";
		desc.DefaultDock = EPanelDock::Floating;
		desc.DefaultSize = ImVec2(760.0f, 460.0f);
		desc.DefaultOpen = false;
		// A dialog over the editor, not part of its layout
		desc.WindowFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking;
		return desc;
	}

	void SettingsPanel::Initialize(EditorContext& context)
	{
		m_pages = SettingsPageRegistry::CreateAll();
	}

	void SettingsPanel::Draw(EditorContext& context)
	{
		if (m_pages.empty())
		{
			ImGui::TextDisabled("No settings pages are registered");
			return;
		}

		DrawPageList();
		ImGui::SameLine();
		DrawPage(context);
	}

	void SettingsPanel::DrawPageList()
	{
		const ImVec2 size(ImGui::GetFontSize() * k_pageListWidth, 0.0f);
		if (ImGui::BeginChild("##Pages", size, ImGuiChildFlags_Borders))
		{
			for (size_t i = 0; i < m_pages.size(); ++i)
			{
				ImGui::PushID(static_cast<int>(i));
				if (ImGui::Selectable(m_pages[i]->GetName().c_str(), i == m_selectedPage))
				{
					m_selectedPage = i;
				}
				ImGui::PopID();
			}
		}
		ImGui::EndChild();
	}

	void SettingsPanel::DrawPage(EditorContext& context)
	{
		if (ImGui::BeginChild("##Page", ImVec2(0.0f, 0.0f), ImGuiChildFlags_AlwaysUseWindowPadding))
		{
			SettingsPage& page = *m_pages[m_selectedPage];

			ImGui::PushFont(nullptr, k_headingFontSize);
			ImGui::TextUnformatted(page.GetName().c_str());
			ImGui::PopFont();
			ImGui::Separator();
			ImGui::Spacing();

			page.Draw(context);
		}
		ImGui::EndChild();
	}
}
