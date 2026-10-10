#include <UI/Panels/LogPanel.h>

#include <EditorApp.h>

#include <DebugUtils/DebugServices.h>

#include <imGUI/imgui.h>

#define MAX_LOG_SIZE 256

namespace Editor
{
	void LogPanel::Display()
	{
		ImGui::Begin("Log", nullptr, ImGuiWindowFlags_NoCollapse);

		const std::vector<DebugServices::LogEntry>& logEntries = DebugServices::Get().GetLogEntries();
		for (auto& logItem : logEntries)
		{
			ImVec4 imColor(logItem.TextColor.X(), logItem.TextColor.Y(), logItem.TextColor.Z(), 1.0f);
			// Log text (e.g. build output) can contain '%', so never use it as the format string.
			ImGui::TextColored(imColor, "%s", logItem.Text.c_str());
		}

		// Entries can arrive from anywhere (RZE_LOG, build output), so follow the count rather than AddEntry.
		static size_t s_lastEntryCount = 0;
		if (logEntries.size() > s_lastEntryCount)
		{
			ImGui::SetScrollHereY();
		}
		s_lastEntryCount = logEntries.size();

		ImGui::End();
	}

	void LogPanel::AddEntry(const std::string& msg)
	{
		DebugServices::Get().Trace(LogChannel::Info, msg);
	}
}