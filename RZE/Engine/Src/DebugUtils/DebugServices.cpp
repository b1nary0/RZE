#include <StdAfx.h>
#include <DebugUtils/DebugServices.h>

#include <imGUI/imgui.h>

#include <EngineCore/Engine.h>

std::deque<DebugServices::LogEntry> DebugServices::mLogEntries;
std::vector<DebugServices::LogEntry> DebugServices::mDataEntries;

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

void DebugServices::AddData(const std::string& text, const Vector3D& color)
{
	mDataEntries.emplace_back();
	LogEntry& entry = mDataEntries.back();

	entry.Text = text;
	entry.TextColor = color;
}

void DebugServices::Initialize()
{

}

void DebugServices::Display()
{
	RenderLog();
	RenderData();
}

void DebugServices::HandleScreenResize(const Vector2D& windowSize)
{
	ImGuiIO& io = ImGui::GetIO();
	
	io.DisplaySize.x = windowSize.X();
	io.DisplaySize.y = windowSize.Y();
}

void DebugServices::RenderLog()
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

void DebugServices::RenderData()
{
	ImGui::SetNextWindowPos(ImVec2(RZE_Engine::Get()->GetMainWindowSize().X() - 280, 50));
	ImGui::SetNextWindowSize(ImVec2(250, 100));
	ImGui::Begin("Data");

	for (auto& dataItem : mDataEntries)
	{
		ImVec4 imColor(dataItem.TextColor.X(), dataItem.TextColor.Y(), dataItem.TextColor.Z(), 1.0f);
		ImGui::TextColored(imColor, dataItem.Text.c_str());
	}
	mDataEntries.clear();

	ImGui::End();
}
