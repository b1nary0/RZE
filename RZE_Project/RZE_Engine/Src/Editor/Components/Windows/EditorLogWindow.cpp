#include <StdAfx.h>

#include <Editor/Components/Windows/EditorLogWindow.h>

EditorLogWindow::EditorLogWindow()
{
	SetMaxItemCount(128);
}

EditorLogWindow::EditorLogWindow(const std::string& componentName)
	: EditorUIComponent(componentName)
{
	SetMaxItemCount(128);
}

EditorLogWindow::~EditorLogWindow()
{
}

void EditorLogWindow::AddItem(const std::string& item)
{
	if (mItems.size() >= mMaxItems)
	{
		mItems.pop_front();
	}

	mItems.push_back(item);
}

const std::deque<std::string>& EditorLogWindow::GetItems()
{
	return mItems;
}

void EditorLogWindow::SetMaxItemCount(U32 maxItems)
{
	mMaxItems = maxItems;
}

U32 EditorLogWindow::GetMaxItemCount()
{
	return mMaxItems;
}
