#include <UI/Panels/LogPanel.h>

#include <UI/Framework/PanelRegistry.h>

#include <DebugUtils/DebugServices.h>

#include <ImGui/imgui.h>

namespace Editor
{
	REGISTER_EDITOR_PANEL(LogPanel);

	PanelDesc LogPanel::Describe()
	{
		PanelDesc desc;
		desc.Name = "Log";
		desc.MenuPath = "View/Windows/Log";
		desc.DefaultDock = EPanelDock::BottomRight;
		desc.WindowFlags = ImGuiWindowFlags_NoCollapse;
		return desc;
	}

	void LogPanel::Draw(EditorContext& context)
	{
		// As of the last frame drawn, before anything new is added below; also holds while the panel was hidden
		const bool wasAtNewest = ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 1.0f;

		const std::vector<DebugServices::LogEntry>& logEntries = DebugServices::Get().GetLogEntries();
		for (auto& logItem : logEntries)
		{
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
