#include <StdAfx.h>
#include <Editor/Widgets/LogWidget.h>

Vector3D LogWidget::mDefaultLogColor = Vector3D(1.0f, 1.0f, 0.0f);

#define MAX_LOG_SIZE 10

void LogWidget::Display()
{
	ImGui::Begin("Log", nullptr, ImVec2(500, 250));

	for (auto& logItem : mLogEntries)
	{
		ImVec4 imColor(logItem.TextColor.X(), logItem.TextColor.Y(), logItem.TextColor.Z(), 1.0f);
		ImGui::TextColored(imColor, logItem.Text.c_str());
	}

	ImGui::SetScrollHere();
	ImGui::End();
}

void LogWidget::AddEntry(const std::string& text, const Vector3D& color /*= mDefaultLogColor*/)
{
	LogEntry entry;
	entry.Text = text;
	entry.TextColor = color;

	if (mLogEntries.size() < MAX_LOG_SIZE)
	{
		mLogEntries.push_back(entry);
	}
	else
	{
		mLogEntries.pop_front();
		mLogEntries.push_back(entry);
	}
}
