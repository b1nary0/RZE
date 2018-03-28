#include <StdAfx.h>
#include <Editor/Widgets/GameViewWidget.h>

GameViewWidget::GameViewWidget()
{

}

GameViewWidget::~GameViewWidget()
{
	// #TODO(Josh) Really need to start the smart ptr pass to avoid stuff like this
	if (mRTT)
	{
		delete mRTT;
	}
}

void GameViewWidget::Initialize()
{
	mRTT = new Diotima::GLRenderTargetTexture();
	mRTT->SetWidth(1280);
	mRTT->SetHeight(720);

	mRTT->Initialize();
}

void GameViewWidget::Display()
{
	ImGui::SetNextWindowSize(ImVec2(mRTT->GetWidth(), mRTT->GetHeight()));
	if (ImGui::Begin("GameView"))
	{
		ImGui::Image((void*)mRTT->GetTextureID(), ImVec2(mRTT->GetWidth(), mRTT->GetHeight()));

		ImGui::End();
	}
}
