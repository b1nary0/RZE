#pragma once

#include <Widgets/EditorWidget.h>

class MainMenuWidget : public IEditorWidget
{
public:
	MainMenuWidget();
	virtual ~MainMenuWidget();

	// IEditorWidget interface
public:
	void Initialize() override;
	void Display() override;
};
