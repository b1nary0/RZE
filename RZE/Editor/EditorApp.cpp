#include <EditorApp.h>

#include <Diotima/Graphics/Texture2D.h>

#include <ECS/Components/CameraComponent.h>
#include <ECS/Components/LightSourceComponent.h>
#include <ECS/Components/MeshComponent.h>
#include <ECS/Components/TransformComponent.h>
#include <ECS/Components/MaterialComponent.h>

#include <ECS/Systems/FreeCameraSystem.h>


void RZE_Editor::Start()
{
	RZE_Application::Start();
}

void RZE_Editor::Update()
{
	PreUpdate();

	// #TODO(Josh) This should change when a proper render system pipeline is in place. For now, that'll do donkey.
	RZE().GetRenderer()->RenderToTexture(GetSceneViewWidget().GetRTT());

	Display();
}

void RZE_Editor::ShutDown()
{

}

bool RZE_Editor::ProcessInput(const InputHandler& handler)
{
	ImGuiIO& io = ImGui::GetIO();
	io.DeltaTime = static_cast<float>(RZE_Application::RZE().GetDeltaTime());
	io.MousePos = ImVec2(handler.GetMouseState().CurPosition.X(), handler.GetMouseState().CurPosition.Y());

	for (int i = 0; i < 3; ++i)
	{
		io.MouseDown[i] = handler.GetMouseState().CurMouseBtnStates[i];
	}

	if (mSceneView.IsHovered())
	{
		return true;
	}

	return false;
}

void RZE_Editor::RegisterInputEvents(InputHandler& inputHandler)
{

}

void RZE_Editor::Initialize()
{
	RZE_Application::Initialize();

	SetupStyle();

	mMainMenu.Initialize();
	mLog.Initialize();
	mSceneView.Initialize();
	mHierarchyView.Initialize();

	RZE().GetActiveScene().GetEntityHandler().AddSystem<FreeCameraSystem>();
}

void RZE_Editor::PreUpdate()
{
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
		mHierarchyView.Display();

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
