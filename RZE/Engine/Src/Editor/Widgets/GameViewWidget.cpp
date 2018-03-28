#include <StdAfx.h>
#include <Editor/Widgets/GameViewWidget.h>

GameViewWidget::GameViewWidget()
{

}

GameViewWidget::~GameViewWidget()
{

}

void GameViewWidget::Initialize()
{
	mRTT = new Diotima::GLRenderTargetTexture();
	mRTT->SetHeight(1920);
	mRTT->SetWidth(1080);

	mRTT->Initialize();
}

void GameViewWidget::Display()
{
	ImGui::SetNextWindowSize(ImVec2(1920, 1080));
	if (ImGui::Begin("GameView"))
	{
		ImGui::Image((void*)mRTT->GetTextureID(), ImVec2(1920, 1080));

		ImGui::End();
	}
}
