#pragma once

#include <Utils/DebugUtils/Debug.h>

class IEditorWidget
{
public:
	IEditorWidget() = default;
	virtual ~IEditorWidget() = default;

	virtual void Initialize() = 0;
	virtual void Display() = 0;

	void AddChild(IEditorWidget* child)
	{
		AssertNotNull(child);
		mChildren.push_back(child);
	}

protected:
	std::vector<IEditorWidget*> mChildren;
};