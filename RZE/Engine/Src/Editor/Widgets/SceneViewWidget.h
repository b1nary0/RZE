#pragma once

#include <Editor/Widgets/EditorWidget.h>

#include <Diotima/Graphics/RenderTarget.h>

class SceneViewWidget : public IEditorWidget
{
public:
	SceneViewWidget();
	virtual ~SceneViewWidget();

	virtual void Initialize() override;
	virtual void Display() override;

public:
	Diotima::RenderTargetTexture* GetRTT() { return mRTT; }

private:
	Diotima::RenderTargetTexture* mRTT;
};