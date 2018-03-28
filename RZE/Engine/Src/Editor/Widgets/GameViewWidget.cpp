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
	ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x / 2, 20.0f), 0, ImVec2(0.5f, 0.0f)); // #TODO(Josh) Need to find solution for the magic numbers here (20.0f) which put this window below the main menu bar
	if (ImGui::Begin("GameView", NULL, ImGuiWindowFlags_NoScrollbar)) // #TODO(Josh) Why does the window need scroll bar?
	{
		ImGui::Image((void*)mRTT->GetTextureID(), ImVec2(size.X(), size.Y()), ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f));

		ImGui::End();
	}
}
