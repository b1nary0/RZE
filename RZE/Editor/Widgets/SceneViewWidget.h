#pragma once

#include <Widgets/EditorWidget.h>

#include <Diotima/Graphics/RenderTarget.h>

class SceneViewWidget : public IEditorWidget
{
public:
	SceneViewWidget();
	virtual ~SceneViewWidget();

	virtual void Initialize() override;
	virtual void Display() override;

public:
	Diotima::RenderTargetTexture* GetRTT() const { return mRTT; }

private:
	Diotima::RenderTargetTexture* mRTT;
};