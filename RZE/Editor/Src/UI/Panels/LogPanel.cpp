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
		desc.DefaultDock = EPanelDock::Bottom;
		desc.WindowFlags = ImGuiWindowFlags_NoCollapse;
		return desc;
	}

	void LogPanel::Update(EditorContext& context)
	{
		const size_t entryCount = DebugServices::Get().GetLogEntries().size();
		m_hasNewEntries = entryCount > m_lastEntryCount;
		m_lastEntryCount = entryCount;
	}

	void LogPanel::Draw(EditorContext& context)
	{
		const std::vector<DebugServices::LogEntry>& logEntries = DebugServices::Get().GetLogEntries();
		for (auto& logItem : logEntries)
		{
			ImVec4 imColor(logItem.TextColor.X(), logItem.TextColor.Y(), logItem.TextColor.Z(), 1.0f);
			// Log text (e.g. build output) can contain '%', so never use it as the format string.
			ImGui::TextColored(imColor, "%s", logItem.Text.c_str());
		}

		if (m_hasNewEntries)
		{
			ImGui::SetScrollHereY();
		}
	}
}
