#pragma once

#include <Editor/Widgets/SceneViewWidget.h>
#include <Editor/Widgets/LogWidget.h>
#include <Editor/Widgets/MainMenuWidget.h>

class RZE_Editor
{
	friend class RZE_Engine;

public:
	RZE_Editor() = default;
	~RZE_Editor() = default;

	LogWidget& GetLogWidget() { return mLog; }
	SceneViewWidget& GetSceneViewWidget() { return mSceneView; }

	void Initialize();
	void PreUpdate();
	void Display();

private:
	void SetupStyle();

private:
	MainMenuWidget mMainMenu;
	LogWidget mLog;
	SceneViewWidget mSceneView;
};