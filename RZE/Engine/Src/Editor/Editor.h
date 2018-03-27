#pragma once

#include <Editor/Widgets/LogWidget.h>

class RZE_Editor
{
	friend class RZE_Engine;

public:
	RZE_Editor() = default;
	~RZE_Editor() = default;

	LogWidget& GetLogWidget() { return mLogWidget; }

	void Initialize();
	void Display();

private:
	LogWidget mLogWidget;
};