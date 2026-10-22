#include <UI/Panels/LogPanel.h>

#include <UI/Framework/PanelRegistry.h>

#include <DebugUtils/DebugServices.h>

#include <ImGui/imgui.h>

namespace Editor
{
	namespace
	{
		constexpr ImVec4 k_filterBarColour = ImVec4(0.19f, 0.19f, 0.19f, 1.0f);
	}

	REGISTER_EDITOR_PANEL(LogPanel);

	PanelDesc LogPanel::Describe()
	{
		PanelDesc desc;
		desc.Name = "Log";
		desc.MenuPath = "View/Windows/Log";
		desc.DefaultDock = EPanelDock::BottomRight;
		// The filter sits in the menu bar, set apart from the entries and not scrolled with them
		desc.WindowFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_MenuBar;
		// The same space around its controls as the scene view's toolbar
		desc.MenuBarPadding = 6.0f;
		desc.MenuBarSeparator = true;
		return desc;
	}

	void LogPanel::PushWindowStyle(PanelWindowStyle& style) const
	{
		// Lighter than the entries below and darker than the tab above, so the filter reads as its own strip
		style.Push(ImGuiCol_MenuBarBg, k_filterBarColour);
	}

	void LogPanel::DrawMenuBar(EditorContext& context)
	{
		m_filter.Draw();
	}

	void LogPanel::Draw(EditorContext& context)
	{
		// As of the last frame drawn, before anything new is added below; also holds while the panel was hidden
		const bool wasAtNewest = ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 1.0f;

		const std::vector<DebugServices::LogEntry>& logEntries = DebugServices::Get().GetLogEntries();
		for (auto& logItem : logEntries)
		{
			if (!m_filter.IsShown(logItem))
			{
				continue;
			}

			ImVec4 imColor(logItem.TextColor.X(), logItem.TextColor.Y(), logItem.TextColor.Z(), 1.0f);
			// Log text (e.g. build output) can contain '%', so never use it as the format string.
			ImGui::TextColored(imColor, "%s", logItem.Text.c_str());
		}

		// Scrolled up to read something: leave it there
		if (wasAtNewest)
		{
			ImGui::SetScrollHereY(1.0f);
		}
	}
}
