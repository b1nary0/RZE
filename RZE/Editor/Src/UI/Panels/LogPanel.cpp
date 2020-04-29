#include <UI/Panels/LogPanel.h>

#include <EditorApp.h>

#include <imGUI/imgui.h>

#define MAX_LOG_SIZE 256

namespace Editor
{
	static bool gShouldSetScroll = false;
	void LogPanel::Display()
	{
		ImGui::Begin("Log", nullptr, ImGuiWindowFlags_NoCollapse);

		for (auto& logItem : mEntries)
		{
			ImVec4 imColor(1.0f, 1.0f, 1.0f, 1.0f);
			ImGui::TextColored(imColor, logItem.Text.c_str());
		}

		if (gShouldSetScroll)
		{
			ImGui::SetScrollHere();
			gShouldSetScroll = false;
		}

		ImGui::End();

	}

	void LogPanel::AddEntry(const std::string& msg)
	{
		LogEntry entry;
		entry.Text = msg;
		if (mEntries.size() < MAX_LOG_SIZE)
		{
			mEntries.push_back(entry);
		}
		else
		{
			mEntries.pop_front();

			entry.Text = msg;
			mEntries.push_back(entry);
		}

		gShouldSetScroll = true;
	}

}