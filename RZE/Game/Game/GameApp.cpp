#include "GameApp.h"

#include <RZE.h>

#include <Graphics/RenderEngine.h>

#include <Rendering/Graphics/RenderTarget.h>

#include <Utils/DebugUtils/Debug.h>
#include <Utils/Platform/FilePath.h>

#ifdef IMGUI_ENABLED
#include <imGUI/imgui.h>
#include <Graphics/RenderStages/ImGuiRenderStage.h>
#endif

#include <Graphics/RenderStages/FinalRenderTargetStage.h>

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

	CreateRenderTarget(GetWindow()->GetDimensions());

	RZE().GetRenderEngine().SetRenderTarget(m_renderTarget.get());

	// #TODO #BIGBUG
	// This function doesn't get called on reload of a scene. Maybe this should be set in the scene itself (the systems, etc)
	// If we load something that _isnt_ the scene below during runtime, EntitySystem::Initialize isnt called.

	GameScene& activeScene = RZE().GetActiveScene();
	// #TODO(Josh::Putting this comment here because too lazy to do it elsewhere. Hopefully I find it later:
	//             -- Need to add at least commandline arg like -startscene or something)
	
	activeScene.Load(FilePath("Assets/Scenes/RenderTest.scene"));
	
#ifdef IMGUI_ENABLED
	RZE::GetRenderEngine().AddRenderStage<ImGuiRenderStage>();
#endif
}

void GameApp::Start()
{
	RZE_Application::Start();
}

void GameApp::Update()
{
	RZE_Application::Update();

#ifdef IMGUI_ENABLED
	ImGui::ShowDemoWindow();
#endif
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

void GameApp::OnWindowResize(const Vector2D& newSize)
{
	m_renderTarget.reset();

	CreateRenderTarget(GetWindow()->GetDimensions());
	RZE().GetRenderEngine().SetRenderTarget(m_renderTarget.get());

	RZE().GetRenderEngine().SetViewportSize(newSize);
}

void GameApp::CreateRenderTarget(const Vector2D& dimensions)
{
	AssertNull(m_renderTarget);

	const U32 renderTargetWidth = static_cast<U32>(dimensions.X());
	const U32 renderTargetHeight = static_cast<U32>(dimensions.Y());
	m_renderTarget = std::make_unique<Rendering::RenderTargetTexture>(renderTargetWidth, renderTargetHeight);

	m_renderTarget->Initialize();
}
