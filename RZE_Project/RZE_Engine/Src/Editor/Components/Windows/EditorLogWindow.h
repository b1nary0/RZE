#pragma once

#include <Editor/Components/EditorUIComponent.h>

class EditorLogWindow : public EditorUIComponent
{
public:
	EditorLogWindow();
	EditorLogWindow(const std::string& componentName);
	virtual ~EditorLogWindow();
};