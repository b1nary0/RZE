#include <StdAfx.h>
#include <DebugUtils/DebugServices.h>

#include <imGUI/imgui.h>

std::vector<DebugServices::LogEntry> DebugServices::mLogEntries;

DebugServices::DebugServices()
{
}

void DebugServices::AddLog(const std::string& text, const Vector3D& color)
{
	mLogEntries.emplace_back();

	LogEntry& entry = mLogEntries.back();
	entry.Text = text;
	entry.TextColor = color;
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

	mLogEntries.clear();
}
