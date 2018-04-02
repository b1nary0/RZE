#include <StdAfx.h>
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

	TestInitializeWhileNoSceneFile();
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

// This only exists to create/demonstrate scene difference, because there is no concept of a scene file (this is basically prep for it)
void RZE_Editor::TestInitializeWhileNoSceneFile()
{
	// #TODO(Josh) editor should do additional logging to the log widget on top of forwarding it to engine... or something
	mLog.AddEntry("Press 6 to load a model.", Vector3D(1.0f, 1.0f, 0.0f));

	GameScene& scene = RZE_Application::RZE().GetActiveScene();

	// ALL TEST CODE
	scene.GetEntityHandler().AddSystem<FreeCameraSystem>();

	Apollo::EntityID floor = scene.GetEntityHandler().CreateEntity();
	scene.GetEntityHandler().AddComponent<MeshComponent>(floor, FilePath("Engine/Assets/3D/Cube.obj"));
	scene.GetEntityHandler().AddComponent<TransformComponent>(floor, Vector3D(-5.0f, -5.5f, -5.0f), Quaternion(), Vector3D(10.0f, 0.5f, 10.0f));

	Apollo::EntityID lightSource = scene.GetEntityHandler().CreateEntity();
	scene.GetEntityHandler().AddComponent<MeshComponent>(lightSource, FilePath("Engine/Assets/3D/Cube.obj"));
	scene.GetEntityHandler().AddComponent<LightSourceComponent>(lightSource, Vector3D(0.8f, 0.8f, 0.8f), 1.0f);
	scene.GetEntityHandler().AddComponent<TransformComponent>(lightSource, Vector3D(0.0f, 10.0f, 8.0f), Quaternion(), Vector3D(1.0f));

	Apollo::EntityID camera = scene.GetEntityHandler().CreateEntity();
	scene.GetEntityHandler().AddComponent<TransformComponent>(camera, Vector3D(-4.0f, 8.0f, 10.0f), Quaternion(), Vector3D(1.0f));
	scene.GetEntityHandler().AddComponent<CameraComponent>(camera);
	CameraComponent* const camComp = scene.GetEntityHandler().GetComponent<CameraComponent>(camera);
	camComp->FOV = 45;
	camComp->NearCull = 0.1f;
	camComp->FarCull = 1000.0f;
	camComp->Forward = Vector3D(0.5f, -0.5f, -1.0f);
	camComp->UpDir = Vector3D(0.0f, 1.0f, 0.0f);
	camComp->AspectRatio = RZE_Application::RZE().GetWindowSize().X() / RZE_Application::RZE().GetWindowSize().Y();

	InputHandler::KeyActionFunc tempKeyFunc([this, &scene](const InputKey& key)
	{
		static float nextXpos = 0.0f;

		if (key.GetKeyCode() == Win32KeyCode::Key_5)
		{
			if (mNanosuits.size())
			{
				scene.GetEntityHandler().DestroyEntity(mNanosuits.back());
				mNanosuits.erase(mNanosuits.end() - 1);

				if (mNanosuits.size() == 0)
				{
					nextXpos = 0.0f;
				}
			}
		}
		else if (key.GetKeyCode() == Win32KeyCode::Key_6)
		{
			mNanosuits.emplace_back(scene.GetEntityHandler().CreateEntity());
			Apollo::EntityID entity = mNanosuits.back();

			scene.GetEntityHandler().AddComponent<MeshComponent>(entity, FilePath("Engine/Assets/3D/Nanosuit/Nanosuit.obj"));
			scene.GetEntityHandler().AddComponent<TransformComponent>(entity, Vector3D(nextXpos, -5.0f, 0.0f), Quaternion(), Vector3D(0.5f));

			nextXpos += 5.0f;
		}
	});
	RZE_Application::RZE().GetInputHandler().BindAction(Win32KeyCode::Key_5, EButtonState::ButtonState_Pressed, tempKeyFunc);
	RZE_Application::RZE().GetInputHandler().BindAction(Win32KeyCode::Key_6, EButtonState::ButtonState_Pressed, tempKeyFunc);

}

