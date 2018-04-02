#pragma once

#include <EngineApp.h>

#include <Widgets/SceneViewWidget.h>
#include <Widgets/LogWidget.h>
#include <Widgets/MainMenuWidget.h>

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

	virtual void RegisterInputEvents(InputHandler& inputHandler) override;

	virtual bool IsEditor() override { return true; }

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