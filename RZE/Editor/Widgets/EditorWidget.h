#pragma once

#include <Utils/DebugUtils/Debug.h>

#include <RZE.h>

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

	inline bool IsFocused() const { return bIsFocused; }
	inline bool IsHovered() const { return bIsHovered; }

protected:
	std::vector<IEditorWidget*> mChildren;

	bool bIsFocused { false };
	bool bIsHovered { false };
};