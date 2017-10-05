#include <StdAfx.h>

#include <imGUI/imgui.h>

#include <EngineUI/Windows/LogWindow.h>

LogWindow::LogWindow()
{
	SetTitle("Log Window");
	SetMaxItemCount(256);
}

LogWindow::~LogWindow()
{
}

void LogWindow::OnCreate()
{
}

void LogWindow::OnDraw()
{
	ImGui::SetNextWindowPos(ImVec2(GetPosition().X(), GetPosition().Y()));
	ImGui::SetNextWindowSize(ImVec2(GetSize().X(), GetSize().Y()));
	ImGui::Begin(GetTitle().c_str());

	for (size_t itemIdx = 0; itemIdx < mItems.size(); itemIdx++)
	{
		const LogInfoItem& item = mItems[itemIdx];
		Vector4D color = GetColorFromPriority(item.mPriority);
		ImGui::TextColored(ImVec4(color.X(), color.Y(), color.Z(), color.W()), item.mBody.c_str());
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

int LogWindow::GetItemSize()
{
	return mItems.size();
}

void LogWindow::Add(const std::string& text, ELogPriorityType::T priority)
{
	if (mItems.size() >= static_cast<size_t>(mMaxItems))
	{
		mItems.pop_front();
	}

	LogInfoItem item;
	item.mPriority = priority;
	item.mBody = text;

	mItems.push_back(item);
}

Vector4D LogWindow::GetColorFromPriority(ELogPriorityType::T priority)
{
	switch (priority)
	{
	case ELogPriorityType::Info:
		return Vector4D(1.0f, 1.0f, 1.0f, 1.0f);

	case ELogPriorityType::Warning:
		return Vector4D(1.0f, 128.0f/255.0f, 0.0f, 1.0f);

	case ELogPriorityType::Error:
		return Vector4D(1.0f, 0.0f, 0.0f, 1.0f);
		break;

	case ELogPriorityType::Debug:
	default:
		return Vector4D(1.0f, 1.0f, 0.0f, 1.0f);
	}
}
