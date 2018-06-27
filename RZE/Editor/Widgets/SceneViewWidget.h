#pragma once

#include <Widgets/EditorWidget.h>

namespace Diotima
{
	class RenderTargetTexture;
}

class SceneViewWidget : public IEditorWidget
{
public:
	SceneViewWidget(Diotima::RenderTarget* renderTarget);
	virtual ~SceneViewWidget();

	virtual void Initialize() override;
	virtual void Display() override;

public:
	Diotima::RenderTarget* GetRenderTarget() const { return pRTT; }

private:
	Diotima::RenderTarget* pRTT;
};