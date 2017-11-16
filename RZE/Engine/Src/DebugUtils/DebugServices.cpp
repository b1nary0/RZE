#include <StdAfx.h>
#include <DebugUtils/DebugServices.h>

#include <imGUI/imgui.h>

std::deque<DebugServices::LogEntry> DebugServices::mLogEntries;

#define MAX_LOG_SIZE 10

DebugServices::DebugServices()
{
}

void DebugServices::AddLog(const std::string& text, const Vector3D& color)
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

void DebugServices::Initialize()
{

}

void DebugServices::Display()
{
	RenderLog();
}

void DebugServices::RenderLog()
{
	for (auto& logItem : mLogEntries)
	{
		ImVec4 imColor(logItem.TextColor.X(), logItem.TextColor.Y(), logItem.TextColor.Z(), 1.0f);
		ImGui::TextColored(imColor, logItem.Text.c_str());
	}

	ImGui::SetScrollHere();
}
