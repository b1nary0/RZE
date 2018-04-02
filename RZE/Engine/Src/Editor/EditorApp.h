#pragma once

#include <EngineApp.h>

#include <Editor/Widgets/SceneViewWidget.h>
#include <Editor/Widgets/LogWidget.h>
#include <Editor/Widgets/MainMenuWidget.h>

class RZE_Editor : RZE_Application
{
	friend class RZE_Engine;

public:
	RZE_Editor() = default;
	~RZE_Editor() = default;

	//
	// RZE_Application interface
	//
public:
	virtual void Start();
	virtual void Initialize();
	virtual void Update();
	virtual void ShutDown();

public:
	LogWidget & GetLogWidget() { return mLog; }
	SceneViewWidget& GetSceneViewWidget() { return mSceneView; }

	void PreUpdate();
	void Display();

private:
	void SetupStyle();

	void TestInitializeWhileNoSceneFile();

private:
	MainMenuWidget mMainMenu;
	LogWidget mLog;
	SceneViewWidget mSceneView;

	std::vector<Apollo::EntityID> mNanosuits;
};