#include <StdAfx.h>
#include <Editor/Widgets/LogWidget.h>

Vector3D LogWidget::mDefaultLogColor = Vector3D(1.0f, 1.0f, 0.0f);

#define MAX_LOG_SIZE 128

void LogWidget::Initialize()
{
}

void LogWidget::Display()
{
	const Vector2D& winSize = RZE_Application::RZE().GetWindowSize();
	ImGui::SetNextWindowPos(ImVec2(0.f, winSize.Y() - 250));
	ImGui::Begin("Log", nullptr, ImVec2(750, 250), -1.0f, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

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
