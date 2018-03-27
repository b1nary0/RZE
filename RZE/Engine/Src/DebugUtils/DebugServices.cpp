#include <StdAfx.h>
#include <DebugUtils/DebugServices.h>

#include <imGUI/imgui.h>

std::vector<DebugServices::LogEntry> DebugServices::mDataEntries;

#define MAX_LOG_SIZE 10

DebugServices::DebugServices()
{
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

void DebugServices::Display(const Vector2D& windowSize)
{
	RenderData(windowSize);
}

void DebugServices::HandleScreenResize(const Vector2D& windowSize)
{
	ImGuiIO& io = ImGui::GetIO();
	
	io.DisplaySize.x = windowSize.X();
	io.DisplaySize.y = windowSize.Y();
}

void DebugServices::RenderData(const Vector2D& windowSize)
{
	ImGui::SetNextWindowPos(ImVec2(windowSize.X() - 280, 50));
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
