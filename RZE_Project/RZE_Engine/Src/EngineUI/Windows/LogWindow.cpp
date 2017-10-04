#include <StdAfx.h>

#include <imGUI/imgui.h>

#include <EngineUI/Windows/LogWindow.h>

LogWindow::LogWindow()
{
	SetTitle("Log Window");
}

LogWindow::~LogWindow()
{
}

void LogWindow::OnCreate()
{
	mItems.reserve(mMaxItems);
}

void LogWindow::OnDraw()
{
	ImGui::SetNextWindowPos(ImVec2(GetPosition().X(), GetPosition().Y()));
	ImGui::SetNextWindowSize(ImVec2(GetSize().X(), GetSize().Y()));
	ImGui::Begin(GetTitle().c_str());

	for (auto& it = mItems.rbegin(); it != mItems.rend(); ++it)
	{
		ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), (*it).c_str());
	}

	ImGui::End();
}

void LogWindow::SetMaxItemCount(int newMaxItemCount)
{
	mMaxItems = newMaxItemCount;
}

int LogWindow::GetMaxItemCount()
{
	return mMaxItems;
}

void LogWindow::Add(const std::string& item)
{
	// #TODO(Josh) Proper logic for item max overflow
	mItems.push_back(item);
}
