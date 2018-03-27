#pragma once

#include <imGUI/imgui.h>

class IEditorWidget
{
public:
	IEditorWidget() = default;
	virtual ~IEditorWidget() = default;

	virtual void Display() = 0;

protected:
};