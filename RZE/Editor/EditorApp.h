#pragma once

#include <EngineApp.h>

#include <Widgets/HierarchyViewWidget.h>
#include <Widgets/LogWidget.h>
#include <Widgets/MainMenuWidget.h>
#include <Widgets/SceneViewWidget.h>

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

	virtual bool ProcessInput(const InputHandler& handler) override;
	virtual void RegisterInputEvents(InputHandler& inputHandler) override;

	virtual bool IsEditor() override { return true; }

public:
	LogWidget & GetLogWidget() { return mLog; }
	SceneViewWidget& GetSceneViewWidget() { return mSceneView; }

	void PreUpdate();
	void Display();

private:
	void SetupStyle();

private:
	// #TODO(Josh) Need to remove this and have a more structured approach for these. (List of IEditorWidget, etc)
	HierarchyViewWidget mHierarchyView;
	LogWidget mLog;
	MainMenuWidget mMainMenu;
	SceneViewWidget mSceneView;

	std::vector<Apollo::EntityID> mNanosuits;
};