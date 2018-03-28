#pragma once

#include <Editor/Widgets/MainMenuWidget.h>
#include <Editor/Widgets/LogWidget.h>

class RZE_Editor
{
	friend class RZE_Engine;

public:
	RZE_Editor() = default;
	~RZE_Editor() = default;

	LogWidget& GetLogWidget() { return mLog; }

	void Initialize();
	void PreUpdate();
	void Display();

private:
	void SetupStyle();

private:
	MainMenuWidget mMainMenu;
	LogWidget mLog;
};