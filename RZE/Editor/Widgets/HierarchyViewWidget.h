#pragma once

#include <Widgets/EditorWidget.h>

class HierarchyViewWidget : public IEditorWidget
{
public:
	HierarchyViewWidget();
	virtual ~HierarchyViewWidget();

	// IEditorWidget interface
public:
	virtual void Initialize() override;
	virtual void Display() override;

};