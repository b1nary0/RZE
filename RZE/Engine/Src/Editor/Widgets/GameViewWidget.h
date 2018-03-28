#pragma once

#include <Editor/Widgets/EditorWidget.h>

#include <Diotima/Graphics/RenderTarget.h>

class GameViewWidget : public IEditorWidget
{
public:
	GameViewWidget();
	virtual ~GameViewWidget();

	virtual void Initialize() override;
	virtual void Display() override;

public:
	Diotima::RenderTargetTexture* GetRTT() { return mRTT; }

private:
	Diotima::RenderTargetTexture* mRTT;
};