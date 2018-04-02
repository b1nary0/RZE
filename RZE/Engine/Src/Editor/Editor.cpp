#include <StdAfx.h>

#include <Editor/Editor.h>

void RZE_Editor::Start()
{
	RZE_Application::Start();
}

void RZE_Editor::Update()
{
	PreUpdate();

	RZE().GetRenderer()->RenderToTexture(GetSceneViewWidget().GetRTT());

	Display();
}

void RZE_Editor::ShutDown()
{

}

void RZE_Editor::Initialize()
{
	RZE_Application::Initialize();

	SetupStyle();

	mMainMenu.Initialize();
	mLog.Initialize();
	mSceneView.Initialize();
}

void RZE_Editor::PreUpdate()
{
	InputHandler& inputHandler = RZE_Application::RZE().GetInputHandler();

	ImGuiIO& io = ImGui::GetIO();
	io.DeltaTime = static_cast<float>(RZE_Application::RZE().GetDeltaTime());
	io.MousePos = ImVec2(inputHandler.GetMouseState().CurPosition.X(), inputHandler.GetMouseState().CurPosition.Y());

	for (int i = 0; i < 3; ++i)
	{
		io.MouseDown[i] = inputHandler.GetMouseState().CurMouseBtnStates[i];
	}
}

void RZE_Editor::Display()
{
 	ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
 	ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
	if (ImGui::Begin("MainWindow", NULL, ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoNavFocus))
	{
		mMainMenu.Display();
		mLog.Display();
		mSceneView.Display();

		ImGui::End();
	}
}

void RZE_Editor::SetupStyle()
{
	ImGuiStyle& style = ImGui::GetStyle();
	style.WindowRounding = 2.0f;
	style.ScrollbarRounding = 2.0f;
	style.ScrollbarSize = 18.0f;
	style.Alpha = 1.0f;

	style.Colors[ImGuiCol_Border] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
	style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
	style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.2f, 0.2f, 0.2f, 1.0f);
	style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
	style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.15f, 0.15f, 0.15f, 1.0f);
	style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
	style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.1f, 0.1f, 0.1f, 1.0f);
	style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.1f, 0.1f, 0.1f, 1.0f);
	style.Colors[ImGuiCol_TitleBg] = ImVec4(0.1f, 0.1f, 0.1f, 1.0f);
	style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
	style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
	style.Colors[ImGuiCol_WindowBg] = ImVec4(0.2f, 0.2f, 0.2f, 1.0f);
}

