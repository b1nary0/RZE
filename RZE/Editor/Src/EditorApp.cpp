#include <EditorApp.h>

#include <Services/AssetFiles.h>

#include <UI/Framework/EditorStyle.h>
#include <UI/Framework/EditorUI.h>

#include <Asset/AssetPaths.h>

#include <Game/World/GameObject/EditorGameComponentInfoCache.h>
#include <Game/World/GameObjectComponents/EditorCameraComponent.h>
#include <Game/World/GameObjectComponents/TransformComponent.h>

#include <Rendering/Graphics/RenderTarget.h>

#include <Graphics/RenderEngine.h>
#include <Graphics/RenderStages/ImGuiRenderStage.h>

#include <EngineCore/Async/AsyncOperationManager.h>
#include <EngineCore/Input/ImGuiInput.h>

#include <Utils/Platform/CmdLine.h>
#include <Utils/Platform/Filepath.h>

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h> // DockBuilderGetNode

#include <Optick/optick.h>

#include <sstream>

namespace
{
	constexpr char kSceneFileToLoadHack[] = { "RenderTest.scene" };

	// ImGui 1.92+ PushFont() takes a size; use the size the font was loaded at.
	void PushFontAtLoadedSize(ImFont* font)
	{
		ImGui::PushFont(font, font->LegacySize);
	}
}

namespace Editor
{
	EditorApp::EditorApp()
	{
		REGISTER_GAMEOBJECTCOMPONENT(EditorCameraComponent);

		// 0x0100 is the default, so we put the transform component decently high up.
		REGISTER_EDITORCOMPONENTCACHE_ORDERDATA( TransformComponent, 0x00F0 );

		// This would be first if uncommented.
		//REGISTER_EDITORCOMPONENTCACHE_ORDERDATA( RenderComponent, 0x000F );

		// Other examples...
		//REGISTER_EDITORCOMPONENTCACHE_ORDERDATA( EditorCameraComponent, 0x0300 );
		//REGISTER_EDITORCOMPONENTCACHE_ORDERDATA( CameraComponent, 0x000E );
	}

	EditorApp::~EditorApp()
	{
	}

	void EditorApp::Initialize()
	{
		RZE_Application::Initialize();

		const bool isWithEditor = true;
		RZE().GetRenderEngine().AddRenderStage<ImGuiRenderStage>(isWithEditor);

		m_imguiConfigFilepath = Filepath("Config/imgui.ini");
		ImGui::GetIO().IniFilename = m_imguiConfigFilepath.GetAbsolutePath().c_str();

		// The main menu bar is the title bar
		GetWindow()->SetCustomTitleBar(true);
		GetWindow()->Maximize();

		const Vector2D& windowDims = GetWindow()->GetClientSize();
		CreateRenderTarget(windowDims);
		RZE().GetRenderEngine().GetMainView().Target = m_renderTarget.get();

		ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		ImGui::GetIO().KeyRepeatDelay = 0.5f;

		LoadFonts();
		EditorStyle::Apply();

		m_mainMenuBar.Initialize();
		m_bottomDockArea.Initialize();

		m_context.GetPanels().CreatePanels(m_context);

		// Nothing can restore what's left from a session that didn't close cleanly
		AssetFiles::EmptyTrash();
		m_context.GetProjectWatcher().Start();

#ifdef _DEBUG
		// Undo only sees changes made through commands, so catch anything that changes the scene's objects another way
		RZE().GetActiveScene().SetObjectsChangedCallback([this]()
			{
				AssertMsg(m_context.GetCommands().IsSceneEditOpen() || m_context.GetSceneDocument().IsLoading(),
					"Scene objects were added or removed outside a command; in the Editor, change them through one");
			});
#endif

		EditorEvents& events = m_context.GetEvents();
		m_titleSubscriptions.push_back(events.Subscribe<SceneLoadedEvent>([this](const SceneLoadedEvent& event)
			{
				RefreshWindowTitle(event.Document);
			}));
		m_titleSubscriptions.push_back(events.Subscribe<SceneSavedEvent>([this](const SceneSavedEvent& event)
			{
				RefreshWindowTitle(event.Document);
			}));
		m_titleSubscriptions.push_back(events.Subscribe<SceneModifiedStateChangedEvent>([this](const SceneModifiedStateChangedEvent& event)
			{
				RefreshWindowTitle(event.Document);
			}));
	}

	void EditorApp::Start()
	{
		// #TODO Reopen the scene that was open when the editor last closed, rather than a fixed default
		std::string_view startScene;
		const Filepath scenePath = AssetPaths::GetScene(CmdLine::Arguments::Get("-scene", startScene)
			? std::string(startScene)
			: std::string(kSceneFileToLoadHack));
		m_context.GetSceneDocument().Load(scenePath);
	}

	void EditorApp::PreUIUpdate()
	{
		OPTICK_EVENT();

		m_context.Update();
		m_context.GetPanels().UpdateAll(m_context);
	}

	void EditorApp::PreRender()
	{
		OPTICK_EVENT();

		m_context.GetPanels().PreRenderAll(m_context);
	}

	void EditorApp::UpdateUI()
	{
		OPTICK_EVENT();

		ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking;
		window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
		window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

		// Before anything draws, so the status bar and the panels see the same tasks
		m_context.GetTasks().Update(RZE().GetAsyncOperationManager().GetVisibleOperations());

		PushFontAtLoadedSize(m_fontMapping.at("din_bold"));
		m_mainMenuBar.Draw(m_context, *GetWindow());
		m_statusBar.Draw(m_context);
		// Before the main dockspace's panels, which may be docked in it
		m_bottomDockArea.Draw(m_context);

		// What the menu bar, status bar and bottom dock area leave of the window
		const ImGuiViewport* const viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->WorkPos);
		ImGui::SetNextWindowSize(viewport->WorkSize);

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));
		bool show = true;
		ImGui::Begin("RZE_Editor", &show, window_flags);
		ImGui::PopStyleVar(1);
		{
			// No node means imgui.ini had no saved layout for it (missing, or saved by an incompatible ImGui version)
			ImGuiID dockspace_id = ImGui::GetID("MyDockspace");
			if (ImGui::DockBuilderGetNode(dockspace_id) == nullptr)
			{
				m_context.GetPanels().BuildDefaultDockLayout(dockspace_id, ImGui::GetContentRegionAvail());
			}

			ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_PassthruCentralNode;
			ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);

			m_context.GetPanels().DrawAll(m_context);
		}
		ImGui::PopFont();
		ImGui::End();

		// Once every panel has drawn, so none of them sees the scene change partway through the frame
		m_context.GetCommands().Flush(m_context);

		PushFontAtLoadedSize(m_fontMapping.at("din_bold"));
		m_asyncOperationModal.Display(RZE().GetAsyncOperationManager());
		m_context.GetUnsavedChangesPrompt().Display(m_context);
		m_context.GetFileBrowser().Display();
		ImGui::PopFont();
	}

	Filepath EditorApp::GetSplashScreenImage() const
	{
		return Filepath("logo.png");
	}

	std::string EditorApp::GetWindowTitle() const
	{
		return "RZEStudio";
	}

	bool EditorApp::IsReadyToShow() const
	{
		return !m_context.GetSceneDocument().IsLoading();
	}

	bool EditorApp::OnCloseRequested()
	{
		if (!m_context.GetSceneDocument().IsModified())
		{
			return true;
		}

		m_context.GetUnsavedChangesPrompt().Request(m_context, "exiting", []() { RZE().PostExit(); });
		return false;
	}

	void EditorApp::ShutDown()
	{
		RZE().GetActiveScene().SetObjectsChangedCallback(nullptr);
		// While the renderer and resources are still up, so nothing the editor loaded is still held when the resources go
		m_context.GetThumbnails().ShutDown();
		m_context.GetPanels().ShutDownAll(m_context);
		m_context.GetFileBrowser().ShutDown();
		m_mainMenuBar.ShutDown();
		// Undo can't restore deleted files once the editor closes
		AssetFiles::EmptyTrash();
	}

	void EditorApp::RegisterInputEvents(InputHandler& inputHandler)
	{
		// NOTE: This will only get called when we're hovering the SceneView panel
		// SEE: EditorApp::ProcessInput return statement. 
		Functor<void, const InputKey&> keyFunc([this](const InputKey& key)
			{
				if (key.GetKeyCode() == KeyCode::Escape)
				{
					EditorSelection& selection = m_context.GetSelection();
					if (!selection.IsEmpty())
					{
						selection.Clear();
					}
				}
			});
		inputHandler.BindAction(KeyCode::Escape, EButtonState::ButtonState_Pressed, keyFunc);
	}

	InputGate EditorApp::ProcessInput(const InputHandler& handler)
	{
		ImGuiInput::SubmitInput(handler);

		// The scene (editor camera, gizmo shortcuts) only gets input while the Scene view is hovered
		const bool isSceneHovered = m_context.GetSceneViewport().IsHovered;
		return InputGate{ isSceneHovered, isSceneHovered };
	}

	void EditorApp::OnWindowResize(const Vector2D& newSize)
	{
		CreateRenderTarget(newSize);
		RZE().GetRenderEngine().GetMainView().Target = m_renderTarget.get();
	}

	void EditorApp::CreateRenderTarget(const Vector2D& dimensions)
	{
		m_renderTarget.reset();

		m_renderTarget = std::make_unique<Rendering::RenderTargetTexture>(static_cast<U32>(dimensions.X()), static_cast<U32>(dimensions.Y()));
		m_renderTarget->Initialize();
	}

	void EditorApp::LoadFonts()
	{
		ImGuiIO& io = ImGui::GetIO();
		io.Fonts->AddFontDefault();

		Filepath ubuntuRegularPath("Assets/Fonts/Ubuntu-Regular.ttf");
		Filepath ubuntuMediumPath("Assets/Fonts/Ubuntu-Medium.ttf");
		Filepath arialPath("Assets/Fonts/Arial.ttf");
		Filepath consolasPath("Assets/Fonts/Consolas.ttf");
		Filepath liberationRegularPath("Assets/Fonts/LiberationMono-Bold.ttf");
		Filepath dinBoldPath("Assets/Fonts/D-DIN-Bold.otf");

		m_fontMapping.insert({ "ubuntu_medium", io.Fonts->AddFontFromFileTTF(ubuntuMediumPath.GetAbsolutePath().c_str(), 16) });
		m_fontMapping.insert({ "ubuntu_regular", io.Fonts->AddFontFromFileTTF(ubuntuRegularPath.GetAbsolutePath().c_str(), 14) });
		m_fontMapping.insert({ "arial", io.Fonts->AddFontFromFileTTF(arialPath.GetAbsolutePath().c_str(), 15) });
		m_fontMapping.insert({ "consolas", io.Fonts->AddFontFromFileTTF(consolasPath.GetAbsolutePath().c_str(), 14) });
		m_fontMapping.insert({ "liberation_bold", io.Fonts->AddFontFromFileTTF(liberationRegularPath.GetAbsolutePath().c_str(), 15) });
		m_fontMapping.insert({ "din_bold", io.Fonts->AddFontFromFileTTF(dinBoldPath.GetAbsolutePath().c_str(), 14) });
	}

	void EditorApp::RefreshWindowTitle(const SceneDocument& document)
	{
		std::ostringstream ss;
		ss << "RZEStudio - ";
		ss << (document.HasFilePath() ? document.GetFilePath().GetRelativePath() : std::string("Untitled"));
		if (document.IsModified())
		{
			ss << "*";
		}
		GetWindow()->SetTitle(ss.str());
	}

	void EditorApp::ParseArguments(char** arguments, int count)
	{
		RZE_Application::ParseArguments(arguments, count);
	}
}
