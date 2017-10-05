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

	for (size_t itemIdx = 0; itemIdx < mItems.size(); itemIdx++)
	{
		ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), mItems[itemIdx].mBody.c_str());
	}

	ImGui::SetScrollHere();

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

void LogWindow::Add(const std::string& text, ELogPriorityType::T priority)
{
	LogInfoItem item;
	item.mPriority = priority;
	item.mBody = text;
	// #TODO(Josh) Proper logic for item max overflow
	mItems.push_back(item);
}
