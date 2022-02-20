#include <UI/Panels/LogPanel.h>

#include <EditorApp.h>

#include <DebugUtils/DebugServices.h>

#include <imGUI/imgui.h>

#define MAX_LOG_SIZE 256

namespace Editor
{
	static bool gShouldSetScroll = false;
	void LogPanel::Display()
	{
		ImGui::Begin("Log", nullptr, ImGuiWindowFlags_NoCollapse);

		const std::vector<DebugServices::LogEntry> logEntries = DebugServices::Get().GetLogEntries();
		for (auto& logItem : logEntries)
		{
			ImVec4 imColor(logItem.TextColor.X(), logItem.TextColor.Y(), logItem.TextColor.Z(), 1.0f);
			ImGui::TextColored(imColor, logItem.Text.c_str());
		}

		if (gShouldSetScroll)
		{
			ImGui::SetScrollHereY();
			gShouldSetScroll = false;
		}

		ImGui::End();
	}

	void LogPanel::AddEntry(const std::string& msg)
	{
		DebugServices::Get().Trace(LogChannel::Info, msg);
		gShouldSetScroll = true;
	}
}