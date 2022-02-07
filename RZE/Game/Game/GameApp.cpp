#include "GameApp.h"

#include <RZE.h>

#include <Utils/DebugUtils/Debug.h>
#include <Utils/Math/Math.h>
#include <Utils/Platform/FilePath.h>

// GAME
#include <Game/Systems/FirstPersonCameraSystem.h>
#include <Game/Systems/RotateSingleEntitySystem.h>

GameApp::GameApp()
	: RZE_Application()
{
}

GameApp::~GameApp()
{
}

void GameApp::Initialize()
{
	RZE_Application::Initialize();

	// #TODO #BIGBUG
	// This function doesn't get called on reload of a scene. Maybe this should be set in the scene itself (the systems, etc)
	// If we load something that _isnt_ the scene below during runtime, EntitySystem::Initialize isnt called.

	GameScene& activeScene = RZE().GetActiveScene();
	// #TODO(Josh::Putting this comment here because too lazy to do it elsewhere. Hopefully I find it later:
	//             -- Need to add at least commandline arg like -startscene or something)
	
	activeScene.Load(FilePath("Assets/Scenes/RenderTest.scene"));
}

void GameApp::Start()
{
	RZE_Application::Start();
}

void GameApp::Update()
{
	RZE_Application::Update();
}

void GameApp::ShutDown()
{
	RZE_Application::ShutDown();
}

void GameApp::RegisterInputEvents(InputHandler& inputHandler)
{
	Functor<void, const InputKey&> keyFunc([this](const InputKey& key)
	{
		if (key.GetKeyCode() == Win32KeyCode::Escape)
		{
			RZE().PostExit();
		}
		else if (key.GetKeyCode() == Win32KeyCode::F1)
		{
			RZE().SetWindowSize(Vector2D(1280, 720));
		}
		else if (key.GetKeyCode() == Win32KeyCode::F2)
		{
			RZE().SetWindowSize(Vector2D(1600, 900));
		}
		else if (key.GetKeyCode() == Win32KeyCode::F3)
		{
			RZE().SetWindowSize(Vector2D(1920, 1080));
		}
	});
	inputHandler.BindAction(Win32KeyCode::Escape, EButtonState::ButtonState_Pressed, keyFunc);
	inputHandler.BindAction(Win32KeyCode::F1, EButtonState::ButtonState_Pressed, keyFunc);
	inputHandler.BindAction(Win32KeyCode::F2, EButtonState::ButtonState_Pressed, keyFunc);
	inputHandler.BindAction(Win32KeyCode::F3, EButtonState::ButtonState_Pressed, keyFunc);
	inputHandler.BindAction(Win32KeyCode::Key_K, EButtonState::ButtonState_Pressed, keyFunc);
}

bool GameApp::ProcessInput(const InputHandler& handler)
{
#ifdef IMGUI_ENABLED
	ImGuiIO& io = ImGui::GetIO();

	const Vector2D& mousePos = handler.GetProxyMouseState().CurPosition;
	const Vector2D& prevMousePos = handler.GetProxyMouseState().PrevPosition;
	io.MousePos = ImVec2(mousePos.X(), mousePos.Y());
	io.MousePosPrev = ImVec2(prevMousePos.X(), prevMousePos.Y());

	for (U32 mouseBtn = 0; mouseBtn < 3; ++mouseBtn)
	{
		io.MouseDown[mouseBtn] = handler.GetProxyMouseState().CurMouseBtnStates[mouseBtn];
	}

	for (int key = 0; key < MAX_KEYCODES_SUPPORTED; ++key)
	{
		io.KeysDown[key] = handler.GetProxyKeyboardState().IsDownThisFrame(key);
	}

	return false;
#endif

	return RZE_Application::ProcessInput(handler);
}
