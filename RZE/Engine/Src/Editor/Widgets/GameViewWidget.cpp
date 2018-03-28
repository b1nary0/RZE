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
	ImGui::SetNextWindowSize(ImVec2(mRTT->GetWidth(), mRTT->GetHeight() + 21)); // #TODO(Josh) why 21?
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(1.0f, 1.0f));
	if (ImGui::Begin("GameView", NULL, ImGuiWindowFlags_NoResize))
	{
		ImGui::Image((void*)mRTT->GetTextureID(), ImVec2(mRTT->GetWidth(), mRTT->GetHeight()), ImVec2(0, 1), ImVec2(1, 0));

		ImGui::End();
	}
	ImGui::PopStyleVar();
}
