#pragma once

#include <deque>

#include <Editor/Components/EditorUIComponent.h>

#include <Utils/PrimitiveDefs.h>

class EditorLogWindow : public EditorUIComponent
{
public:
	EditorLogWindow();
	EditorLogWindow(const std::string& componentName);
	virtual ~EditorLogWindow();

	void AddItem(const std::string& item);
	const std::deque<std::string>& GetItems();

	void SetMaxItemCount(U32 maxItems);
	U32 GetMaxItemCount();

private:
	U32 mMaxItems;

	std::deque<std::string> mItems;
};