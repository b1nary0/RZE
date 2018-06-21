#pragma once
#include <EngineApp.h>

#include <RZE_EditorConfig.h>

#include <Widgets/HierarchyViewWidget.h>
#include <Widgets/LogWidget.h>
#include <Widgets/MainMenuWidget.h>
#include <Widgets/SceneViewWidget.h>

using namespace RZE_EditorConfig;

class RZE_Editor : public RZE_Application
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
	void PreUpdate();
	void Display();

public:
	template <typename TWidgetType>
	TWidgetType& GetWidget(EWidgetType widgetType)
	{
		AssertNotNull(mWidgetDict[widgetType]);
		TWidgetType* widget = static_cast<TWidgetType*>(mWidgetDict[widgetType]);
		return *widget;
	}

private:
	//
	// #IMPORTANT 06/17/18 :: 10:53PDT
	//
	// These signatures need to be rewritten without widgetType param,
	// but to get working now, leaving it as is because the refactor will be easier within the function
	//
	template <typename TWidgetType>
	TWidgetType& AddWidget(EWidgetType widgetType)
	{
		TWidgetType* widget = new TWidgetType();
		mWidgetDict[widgetType] = widget;
		return *widget;
	}

	void SetupStyle();
	void SetupWidgets();

private:
	std::unordered_map<EWidgetType, IEditorWidget*> mWidgetDict;
};