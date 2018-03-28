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
	Vector2D size(static_cast<float>(mRTT->GetWidth()), static_cast<float>(mRTT->GetHeight()));
	ImGui::SetNextWindowSize(ImVec2(size.X(), size.Y()));
	if (ImGui::Begin("GameView", NULL, ImGuiWindowFlags_NoScrollbar)) // #TODO(Josh) Why does the window need scroll bar?
	{
		ImGui::Image((void*)mRTT->GetTextureID(), ImVec2(size.X(), size.Y()), ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f));

		ImGui::End();
	}
}
